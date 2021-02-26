#include "AudioPort.h"
#include "Utils.h"
#include <vector>
#include <thread>
#include <cmath>

#define DURATION_BUF 0.01
#define REACTION_TIME 0.001

namespace LiveKit
{
	AudioPort::AudioPort()
	{
		m_ref_t = time_micro_sec();
		InitializeCriticalSectionAndSpinCount(&m_cs_writers, 0x00000400);
	}

	AudioPort::~AudioPort()
	{
		m_readers.clear();
		m_writers.clear();
		DeleteCriticalSection(&m_cs_writers);
	}

	class AudioPort::Writer
	{
	public:
		Writer(AudioPort* port, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
			: m_port(port), m_samplerate(samplerate), m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
		{
			m_samples_per_buffer = (int)ceil(DURATION_BUF* (double)samplerate);
			size_t size_loop = (size_t)m_samples_per_buffer*4;
			m_loop_queue.resize(size_loop * 2, 0.0f);
			m_buf.resize(m_samples_per_buffer * 2, 0);

			uint64_t t = rel_time_micro_sec();
			m_last_buffer = (t * m_samplerate) / 1000000 / m_samples_per_buffer;

			m_writing = true;
			m_thread_write = (std::unique_ptr<std::thread>)(new std::thread(thread_write, this));
		}

		~Writer()
		{
			m_writing = false;
			m_thread_write->join();
			m_thread_write = nullptr;
		}

		void set_volume(float vol)
		{
			m_volume = vol;
		}

		void read_add(double t_read, int samplerate, float* buf, size_t size) const
		{
			double pos = t_read * (double)m_samplerate;
			double delta = (double)m_samplerate / (double)samplerate;

			for (size_t i = 0; i < size; i++)
			{
				double p = pos + (double)i*delta;
				size_t i_p = (size_t)p;
				double frac_p = p - (double)i_p;
				size_t p1 = i_p % (m_loop_queue.size() / 2);
				size_t p2 = (i_p + 1) % (m_loop_queue.size() / 2);
				{
					float f1 = m_loop_queue[p1 * 2];
					float f2 = m_loop_queue[p2 * 2];
					buf[i * 2] += (f1 * (1.0f - frac_p) + f2 * frac_p)*m_volume;
				}
				{
					float f1 = m_loop_queue[p1 * 2 + 1];
					float f2 = m_loop_queue[p2 * 2 + 1];
					buf[i * 2 + 1] += (f1 * (1.0f - frac_p) + f2 * frac_p)*m_volume;
				}
			}
		}

	private:
		AudioPort* m_port;
		float m_volume = 1.0f;
		std::vector<float> m_loop_queue;
		std::vector<short> m_buf;
		int m_samplerate;
		int m_samples_per_buffer;
		AudioWriteCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;
		size_t m_last_buffer;

		bool m_writing = false;
		std::unique_ptr<std::thread> m_thread_write;

		uint64_t rel_time_micro_sec()
		{
			return time_micro_sec() - m_port->m_ref_t;
		}

		double rel_time_sec()
		{
			return (double)time_micro_sec() / 1000000.0;
		}

		void write_buf(size_t pos)
		{
			for (size_t i = 0; i < m_samples_per_buffer; i++)
			{
				short v1 = m_buf[i * 2];
				short v2 = m_buf[i * 2 + 1];
				float f1 = (float)v1 / 32768.0f;
				float f2 = (float)v2 / 32768.0f;

				size_t j = (pos + i) % (m_loop_queue.size() / 2);
				m_loop_queue[j * 2] = f1;
				m_loop_queue[j * 2 + 1] = f2;
			}
		}

		static void thread_write(Writer* self)
		{
			bool eof = false;
			while (self->m_writing)
			{
				size_t i_buf = self->m_last_buffer + 1;
				uint64_t t_buf = i_buf * self->m_samples_per_buffer * 1000000 / self->m_samplerate;
				uint64_t t = self->rel_time_micro_sec();
				if (t_buf > t)
					std::this_thread::sleep_for(std::chrono::microseconds(t_buf - t));
				if (eof) break;
				eof = !self->m_callback(self->m_buf.data(), (size_t)self->m_samples_per_buffer, self->m_user_ptr);
				self->write_buf(i_buf*self->m_samples_per_buffer);
				self->m_last_buffer = i_buf;
			}
			self->m_writing = false;
			self->m_eof_callback(self->m_user_ptr);
		}
	};

	void AudioPort::_read(double t_read, int samplerate, float* buf, size_t size) const
	{
		memset(buf, 0, sizeof(float)*size * 2);
		EnterCriticalSection(&m_cs_writers);
		for (auto iter = m_writers.begin(); iter != m_writers.end(); iter++)
		{
			const Writer* writer = iter->second.get();
			writer->read_add(t_read, samplerate, buf, size);
		}
		LeaveCriticalSection(&m_cs_writers);
	}

	class AudioPort::Reader
	{
	public:
		Reader(AudioPort* port, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
			: m_port(port), m_samplerate(samplerate), m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
		{
			m_samples_per_buffer = (int)ceil(DURATION_BUF* (double)samplerate);
			m_fbuf.resize(m_samples_per_buffer * 2, 0);
			m_buf.resize(m_samples_per_buffer * 2, 0);
			uint64_t t = rel_time_micro_sec();
			m_last_buffer = (t * m_samplerate) / 1000000 / m_samples_per_buffer;

			m_reading = true;
			m_thread_read = (std::unique_ptr<std::thread>)(new std::thread(thread_read, this));
		}

		~Reader()
		{
			m_reading = false;
			m_thread_read->join();
			m_thread_read = nullptr;
		}

	private:
		AudioPort* m_port;
		std::vector<float> m_fbuf;
		std::vector<short> m_buf;
		int m_samplerate;
		int m_samples_per_buffer;
		AudioReadCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;
		size_t m_last_buffer;

		bool m_reading = false;
		std::unique_ptr<std::thread> m_thread_read;

		uint64_t rel_time_micro_sec()
		{
			return time_micro_sec() - m_port->m_ref_t;
		}

		double rel_time_sec()
		{
			return (double)time_micro_sec() / 1000000.0;
		}

		static inline short round_clamp(float v)
		{
			v = v * 32768.0f + 0.5f;
			if (v < -32767.0f) v = -32767.0f;
			else if (v > 32767.0f) v = 32767.0f;
			return (short)v;
		}

		void convert_buf()
		{
			for (size_t i = 0; i < m_samples_per_buffer; i++)
			{
				m_buf[i * 2] = round_clamp(m_fbuf[i * 2]);
				m_buf[i * 2 + 1] = round_clamp(m_fbuf[i * 2 + 1]);
			}
		}

		static void thread_read(Reader* self)
		{
			bool eof = false;
			while (self->m_reading)
			{
				size_t i_buf = self->m_last_buffer + 1;
				uint64_t t_buf = i_buf * self->m_samples_per_buffer * 1000000 / self->m_samplerate;
				uint64_t t = self->rel_time_micro_sec();
				if (t_buf > t)
					std::this_thread::sleep_for(std::chrono::microseconds(t_buf - t));

				if (eof) break;

				double t_read = (double)(i_buf * self->m_samples_per_buffer - (uint64_t)(REACTION_TIME*self->m_samplerate + self->m_samples_per_buffer)) / (double)(self->m_samplerate);
				self->m_port->_read(t_read, self->m_samplerate, self->m_fbuf.data(), self->m_samples_per_buffer);
				self->convert_buf();

				eof = !self->m_callback(self->m_buf.data(), self->m_samples_per_buffer, self->m_user_ptr);

				self->m_last_buffer = i_buf;
			}
			self->m_reading = false;
			self->m_eof_callback(self->m_user_ptr);
		}
	};


	int AudioPort::AddWriter(int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		EnterCriticalSection(&m_cs_writers);
		m_writers[m_id_next_writer] = (std::unique_ptr<Writer>)(new Writer(this, samplerate, callback, eof_callback, user_ptr));
		m_id_next_writer++;
		LeaveCriticalSection(&m_cs_writers);
		return m_id_next_writer - 1;
	}

	void AudioPort::RemoveWriter(int id)
	{
		EnterCriticalSection(&m_cs_writers);
		auto iter = m_writers.find(id);
		if (iter != m_writers.end())
			m_writers.erase(iter);
		LeaveCriticalSection(&m_cs_writers);
	}

	void AudioPort::SetVolume(int id, float vol)
	{
		EnterCriticalSection(&m_cs_writers);
		auto iter = m_writers.find(id);
		if (iter != m_writers.end())
			iter->second->set_volume(vol);
		LeaveCriticalSection(&m_cs_writers);
	}

	int AudioPort::AddReader(int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		m_readers[m_id_next_reader] = (std::unique_ptr<Reader>)(new Reader(this, samplerate, callback, eof_callback, user_ptr));
		m_id_next_reader++;
		return m_id_next_reader - 1;
	}

	void AudioPort::RemoveReader(int id)
	{
		auto iter = m_readers.find(id);
		if (iter != m_readers.end())
			m_readers.erase(iter);
	}
}
