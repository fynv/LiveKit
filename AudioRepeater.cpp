#include "AudioRepeater.h"
#include "AudioIO.h"
#include "BufferQueue.h"

namespace LiveKit
{
	class AudioRepeater::AudioRecorder
	{
	public:
		AudioRecorder(int devId)
		{
			m_audio_in = (std::unique_ptr<AudioIn>)(new AudioIn(devId, 44100, callback, eof_callback, this));
		}

		~AudioRecorder()
		{
			m_audio_in = nullptr;
		}

		BufferQueue* get_buffer_queue()
		{
			return &m_buffer_queue;
		}

	private:
		BufferQueue m_buffer_queue;
		std::unique_ptr<AudioIn> m_audio_in;

		static void eof_callback(void* usr_ptr) {}
		static bool callback(const short* buf, size_t buf_size, void* usr_ptr)
		{
			AudioRecorder* self = (AudioRecorder*)usr_ptr;
			self->recordBuf(buf, buf_size);
			return true;
		}

		void recordBuf(const short* buf, size_t buf_size)
		{
			AudioBuffer* newBuf= new AudioBuffer(2, buf_size);
			memcpy(newBuf->data(), buf, buf_size * sizeof(short) * 2);
			m_buffer_queue.PushBuffer(newBuf);
		}

	};

	class AudioRepeater::AudioPlayback
	{
	public:
		AudioPlayback(int devId, BufferQueue* buffer_queue) : m_buffer_queue(buffer_queue)
		{
			m_audio_out = (std::unique_ptr<AudioOut>)(new AudioOut(devId, 44100, callback, eof_callback, this));
		}

		~AudioPlayback()
		{
			m_audio_out = nullptr;
			delete m_buf_in;
		}

	private:
		BufferQueue* m_buffer_queue;
		std::unique_ptr<AudioOut> m_audio_out;
		static void eof_callback(void* usr_ptr) {}

		AudioBuffer* m_buf_in = nullptr;
		int m_in_pos = 0;

		static bool callback(short* buf, size_t buf_size, void* usr_ptr)
		{
			AudioPlayback* self = (AudioPlayback*)usr_ptr;
			self->fillBuf(buf, buf_size);
			return true;
		}

		void fillBuf(short* buf, size_t buf_size)
		{
			int out_pos = 0;
			while (out_pos < buf_size)
			{
				if (m_buf_in != nullptr && m_in_pos < m_buf_in->len())
				{
					int copy_size = m_buf_in->len() - m_in_pos;
					int copy_size_out = buf_size - out_pos;
					if (copy_size > copy_size_out) copy_size = copy_size_out;
					short* p_out = buf + out_pos * 2;
					const short* p_in = (const short*)m_buf_in->data() + m_in_pos * 2;
					memcpy(p_out, p_in, sizeof(short)*copy_size * 2);
					out_pos += copy_size;
					m_in_pos += copy_size;
				}
				else
				{
					delete m_buf_in;
					m_buf_in = m_buffer_queue->PopBuffer();
					m_in_pos = 0;					
				}
			}
		}
	};

	AudioRepeater::AudioRepeater(int audio_device_id_in, int audio_device_id_out)
		: m_audio_recorder(new AudioRecorder(audio_device_id_in))
		, m_audio_playback(new AudioPlayback(audio_device_id_out, m_audio_recorder->get_buffer_queue()))
	{

	}

	AudioRepeater::~AudioRepeater()
	{
		m_audio_playback = nullptr;
	}
}
