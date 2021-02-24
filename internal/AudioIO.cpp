#include "AudioIO.h"
#include <mmsystem.h>
#include <mmreg.h>

namespace LiveKit
{
	std::unordered_map<int, std::unique_ptr<AudioPort>> g_audio_ports;

	inline AudioPort* get_port(int port_id)
	{
		auto iter = g_audio_ports.find(port_id);
		if (iter == g_audio_ports.end())
		{
			g_audio_ports[port_id] = (std::unique_ptr<AudioPort>)(new AudioPort);
		}
		return g_audio_ports[port_id].get();
	}

	inline void erase_port(int port_id)
	{
		auto iter = g_audio_ports.find(port_id);
		if (iter != g_audio_ports.end())
		{
			g_audio_ports.erase(iter);
		}
	}

	class AudioOut::Implementer
	{
	public:
		Implementer() {}
		virtual ~Implementer() {}	
	};

	class AudioOut::ImplPort : public Implementer
	{
	public:
		ImplPort(int audio_device_id, int samplerate, int samples_per_buffer, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
		{
			m_port_id = audio_device_id;
			AudioPort* port = get_port(m_port_id);
			m_writer_id = port->AddWriter(samplerate, samples_per_buffer, callback, eof_callback, user_ptr);
		}

		~ImplPort()
		{
			AudioPort* port = get_port(m_port_id);
			port->RemoveWriter(m_writer_id);
			if (port->IsIdling())
				erase_port(m_port_id);
		}

	private:
		int m_port_id;
		int m_writer_id;

	};

	class AudioOut::ImplMME : public Implementer
	{
	public:
		ImplMME(int audio_device_id, int samplerate, int samples_per_buffer, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
			: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
		{
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = samplerate;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
			format.nBlockAlign = sizeof(short)*format.nChannels;
			format.wBitsPerSample = 16;
			format.cbSize = 0;

			waveOutOpen(&m_WaveOut, audio_device_id, &format, (DWORD_PTR)(SoundOutCallBack), (DWORD_PTR)this, CALLBACK_FUNCTION);

			m_Buffer = new short[samples_per_buffer * format.nChannels * 2];
			m_Buffer1 = m_Buffer;
			m_Buffer2 = m_Buffer + samples_per_buffer * format.nChannels;
			memset(m_Buffer, 0, sizeof(short)*samples_per_buffer * format.nChannels * 2);

			m_callback(m_Buffer1, m_user_ptr);
			m_callback(m_Buffer2, m_user_ptr);

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

		~ImplMME()
		{
			m_isPlaying = false;
			waveOutReset(m_WaveOut);
			waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
			waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
			waveOutClose(m_WaveOut);
			delete[] m_Buffer;
		}


	private:		
		AudioWriteCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;
		
		bool m_isPlaying = true;
		HWAVEOUT m_WaveOut;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR	m_WaveHeader1, m_WaveHeader2;


		static void CALLBACK SoundOutCallBack(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			if (uMsg == WOM_DONE)
			{
				ImplMME* self = (ImplMME*)dwInstance;
				WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
				if (!self->m_isPlaying)
				{
					self->m_eof_callback(self->m_user_ptr);
					return;
				}
				self->m_isPlaying = self->m_callback((short*)pwhr->lpData, self->m_user_ptr);				
				waveOutWrite(hwo, pwhr, sizeof(WAVEHDR));
			}
		}

	};

	AudioOut::AudioOut(int audio_device_id, int samplerate, int samples_per_buffer, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		if (audio_device_id >= 0)
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplMME(audio_device_id, samplerate, samples_per_buffer, callback, eof_callback, user_ptr));
		}
		else
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplPort(audio_device_id, samplerate, samples_per_buffer, callback, eof_callback, user_ptr));
		}

	}

	AudioOut::~AudioOut()
	{

	}

	class AudioIn::Implementer
	{
	public:
		Implementer() {}
		virtual ~Implementer() {}
	};

	class AudioIn::ImplPort : public Implementer
	{
	public:
		ImplPort(int audio_device_id, int samplerate, int samples_per_buffer, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
		{
			m_port_id = audio_device_id;
			AudioPort* port = get_port(m_port_id);
			m_reader_id = port->AddReader(samplerate, samples_per_buffer, callback, eof_callback, user_ptr);
		}

		~ImplPort()
		{
			AudioPort* port = get_port(m_port_id);
			port->RemoveReader(m_reader_id);
			if (port->IsIdling())
				erase_port(m_port_id);
		}

	private:
		int m_port_id;
		int m_reader_id;
	};

	class AudioIn::ImplMME : public Implementer
	{
	public:
		ImplMME(int audio_device_id, int samplerate, int samples_per_buffer, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
			: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
		{
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = 44100;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
			format.nBlockAlign = sizeof(short) * format.nChannels;
			format.wBitsPerSample = 16;
			format.cbSize = 0;

			waveInOpen(&m_WaveIn, audio_device_id, &format, (DWORD_PTR)(s_SoundInCallback), (DWORD_PTR)this, CALLBACK_FUNCTION);

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

		~ImplMME()
		{
			m_isReceiving = false;
			waveInStop(m_WaveIn);
			waveInReset(m_WaveIn);
			waveInUnprepareHeader(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));
			waveInUnprepareHeader(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));
			waveInClose(m_WaveIn);

			delete[] m_Buffer;
		}

	private:
		AudioReadCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;

		bool m_isReceiving = false;
		HWAVEIN m_WaveIn;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR m_WaveHeader1, m_WaveHeader2;

		static void __stdcall s_SoundInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			if (uMsg == WIM_DATA)
			{
				ImplMME* self = (ImplMME*)dwInstance;
				WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
				if (!self->m_isReceiving)
				{
					self->m_eof_callback(self->m_user_ptr);
					return;
				}
				self->m_isReceiving = self->m_callback((short*)pwhr->lpData, self->m_user_ptr);
				waveInAddBuffer(hwi, pwhr, sizeof(WAVEHDR));
			}
		}
	};

	AudioIn::AudioIn(int audio_device_id, int samplerate, int samples_per_buffer, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
	{
		if (audio_device_id >= 0)
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplMME(audio_device_id, samplerate, samples_per_buffer, callback, eof_callback, user_ptr));
		}
		else
		{
			m_impl = (std::unique_ptr<Implementer>)(new ImplPort(audio_device_id, samplerate, samples_per_buffer, callback, eof_callback, user_ptr));
		}
	}

	AudioIn::~AudioIn()
	{

	}
}
