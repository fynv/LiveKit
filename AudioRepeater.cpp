#include "AudioRepeater.h"
#include "AudioIO.h"

#include <queue>
#include <Windows.h>

namespace LiveKit
{
	class Buffer
	{
	public:
		short* m_data;
		Buffer(size_t size)
		{
			m_data = new short[size];
		}
		~Buffer()
		{
			delete[] m_data;
		}
	};

	class BufferQueue
	{
	public:
		BufferQueue()
		{
			InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
			m_hSemaphore = CreateSemaphore(NULL, 0, ~(1 << 31), NULL);
		}

		~BufferQueue()
		{
			while (m_queue.size() > 0)
			{
				Buffer* buf = PopBuffer();
				delete buf;
			}
			CloseHandle(m_hSemaphore);
			DeleteCriticalSection(&m_cs);
		}

		size_t Size()
		{
			return m_queue.size();
		}

		void PushBuffer(Buffer* buf)
		{
			EnterCriticalSection(&m_cs);
			m_queue.push(buf);
			LeaveCriticalSection(&m_cs);
			ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}

		Buffer* PopBuffer()
		{
			WaitForSingleObject(m_hSemaphore, INFINITE);
			EnterCriticalSection(&m_cs);
			Buffer* ret = m_queue.front();
			m_queue.pop();
			LeaveCriticalSection(&m_cs);
			return ret;
		}

	private:
		std::queue<Buffer*> m_queue;
		CRITICAL_SECTION m_cs;
		HANDLE m_hSemaphore;
	};


	class AudioRepeater::AudioRecorder
	{
	public:
		AudioRecorder(int devId, size_t samples_per_buffer) : m_samples_per_buffer(samples_per_buffer)
		{
			m_audio_in = (std::unique_ptr<AudioIn>)(new AudioIn(devId, 44100, samples_per_buffer, callback, eof_callback, this));
		}

		~AudioRecorder()
		{
			m_audio_in = nullptr;
		}

		BufferQueue* get_buffer_queue()
		{
			return &m_buffer_queue;
		}

		BufferQueue* get_recycler()
		{
			return &m_recycler;
		}

	private:
		size_t m_samples_per_buffer;
		BufferQueue m_buffer_queue;
		BufferQueue m_recycler;
		
		std::unique_ptr<AudioIn> m_audio_in;

		static void eof_callback(void* usr_ptr) {}

		static bool callback(const short* buf, void* usr_ptr)
		{
			AudioRecorder* self = (AudioRecorder*)usr_ptr;
			self->recordBuf(buf);
			return true;
		}

		void recordBuf(const short* data)
		{
			Buffer* newBuf;
			if (m_recycler.Size() > 0)
				newBuf = m_recycler.PopBuffer();
			else
				newBuf = new Buffer(m_samples_per_buffer * 2);
			memcpy(newBuf->m_data, data, m_samples_per_buffer * sizeof(short) * 2);
			m_buffer_queue.PushBuffer(newBuf);
		}
	};

	class AudioRepeater::AudioPlayback
	{
	public:
		AudioPlayback(int devId, size_t samples_per_buffer, BufferQueue* buffer_queue, BufferQueue* recycler)
			: m_samples_per_buffer(samples_per_buffer), m_buffer_queue(buffer_queue), m_recycler(recycler)
		{
			m_audio_out = (std::unique_ptr<AudioOut>)(new AudioOut(devId, 44100, samples_per_buffer, callback, eof_callback, this));
		}

		~AudioPlayback()
		{
			
		}


	private:
		size_t m_samples_per_buffer;
		BufferQueue* m_buffer_queue;
		BufferQueue* m_recycler;

		std::unique_ptr<AudioOut> m_audio_out;

		static void eof_callback(void* usr_ptr) {}

		static bool callback(short* buf, void* usr_ptr)
		{
			AudioPlayback* self = (AudioPlayback*)usr_ptr;
			self->fillBuf(buf);
			return true;
		}

		void fillBuf(short* data)
		{
			if (m_buffer_queue->Size() > 0)
			{
				Buffer* buf = m_buffer_queue->PopBuffer();
				memcpy(data, buf->m_data, m_samples_per_buffer * sizeof(short) * 2);
				m_recycler->PushBuffer(buf);
			}
		}
	};


	AudioRepeater::AudioRepeater(int audio_device_id_in, int audio_device_id_out)
		: m_audio_recorder(new AudioRecorder(audio_device_id_in, 4096))
		, m_audio_playback(new AudioPlayback(audio_device_id_out, 4096, m_audio_recorder->get_buffer_queue(), m_audio_recorder->get_recycler()))
	{
		
	}

	AudioRepeater::~AudioRepeater()
	{
		m_audio_playback = nullptr;
	}
}
