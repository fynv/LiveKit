#include "AudioRepeater.h"

#include <queue>
#include <Windows.h>
#include <mmsystem.h>
#include <mmreg.h>

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
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = 44100;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
			format.nBlockAlign = sizeof(short) * format.nChannels;
			format.wBitsPerSample = 16;
			format.cbSize = 0;

			waveInOpen(&m_WaveIn, devId, &format, (DWORD_PTR)(s_SoundInCallback), (DWORD_PTR)this, CALLBACK_FUNCTION);

			m_Buffer = new short[samples_per_buffer * format.nChannels * 2];
			m_Buffer1 = m_Buffer;
			m_Buffer2 = m_Buffer + samples_per_buffer * format.nChannels;
			memset(m_Buffer, 0, sizeof(short)*samples_per_buffer * format.nChannels * 2);

			m_WaveHeader1.lpData = (char *)m_Buffer1;
			m_WaveHeader1.dwBufferLength = (DWORD)(samples_per_buffer * format.nChannels * sizeof(short));
			m_WaveHeader1.dwFlags = 0;
			waveInPrepareHeader(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));
			waveInAddBuffer(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));

			m_WaveHeader2.lpData = (char *)m_Buffer2;
			m_WaveHeader2.dwBufferLength = (DWORD)(samples_per_buffer* format.nChannels * sizeof(short));
			m_WaveHeader2.dwFlags = 0;
			waveInPrepareHeader(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));
			waveInAddBuffer(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));

			m_isReceiving = true;
			waveInStart(m_WaveIn);			
		}

		~AudioRecorder()
		{
			m_isReceiving = false;
			waveInStop(m_WaveIn);
			waveInReset(m_WaveIn);
			waveInUnprepareHeader(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));
			waveInUnprepareHeader(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));
			waveInClose(m_WaveIn);

			delete[] m_Buffer;
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

		bool m_isReceiving = false;
		HWAVEIN m_WaveIn;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR m_WaveHeader1, m_WaveHeader2;

		static void __stdcall s_SoundInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			if (uMsg == WIM_DATA)
			{
				AudioRecorder* self = (AudioRecorder*)dwInstance;
				WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
				if (!self->m_isReceiving) return;
				self->recordBuf((short*)pwhr->lpData);
				waveInAddBuffer(hwi, pwhr, sizeof(WAVEHDR));
			}
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
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = 44100;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
			format.nBlockAlign = sizeof(short)*format.nChannels;
			format.wBitsPerSample = 16;
			format.cbSize = 0;

			waveOutOpen(&m_WaveOut, devId, &format, (DWORD_PTR)(SoundOutCallBack), (DWORD_PTR)this, CALLBACK_FUNCTION);

			m_Buffer = new short[samples_per_buffer * format.nChannels * 2];
			m_Buffer1 = m_Buffer;
			m_Buffer2 = m_Buffer + samples_per_buffer * format.nChannels;
			memset(m_Buffer, 0, sizeof(short)*samples_per_buffer * format.nChannels * 2);

			fillBuf(m_Buffer1);
			fillBuf(m_Buffer2);

			m_WaveHeader1.lpData = (char *)m_Buffer1;
			m_WaveHeader1.dwBufferLength = samples_per_buffer * format.nChannels * sizeof(short);
			m_WaveHeader1.dwFlags = 0;
			waveOutPrepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
			waveOutWrite(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));

			m_WaveHeader2.lpData = (char *)m_Buffer2;
			m_WaveHeader2.dwBufferLength = samples_per_buffer * format.nChannels * sizeof(short);
			m_WaveHeader2.dwFlags = 0;
			waveOutPrepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
			waveOutWrite(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
		}


	private:
		size_t m_samples_per_buffer;
		BufferQueue* m_buffer_queue;
		BufferQueue* m_recycler;

		bool m_isPlaying = true;
		HWAVEOUT m_WaveOut;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR	m_WaveHeader1, m_WaveHeader2;

		static void CALLBACK SoundOutCallBack(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			if (uMsg == WOM_DONE)
			{
				AudioPlayback* self = (AudioPlayback*)dwInstance;
				WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
				if (!self->m_isPlaying) return;
				self->fillBuf((short*)pwhr->lpData);
				waveOutWrite(hwo, pwhr, sizeof(WAVEHDR));
			}
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
