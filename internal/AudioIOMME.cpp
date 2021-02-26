#include "AudioIOMME.h"

#pragma comment(lib, "winmm.lib")

namespace LiveKit
{
	const std::vector<std::string>& AudioOut::ImplMME::s_get_list_audio_devices(int* id_default)
	{
		static std::vector<std::string> s_list_devices;
		if (s_list_devices.size() == 0)
		{
			unsigned num_dev = waveOutGetNumDevs();
			WAVEOUTCAPS waveOutDevCaps;
			for (unsigned i = 0; i < num_dev; i++)
			{
				waveOutGetDevCaps(i, &waveOutDevCaps, sizeof(WAVEOUTCAPS));
				s_list_devices.push_back(waveOutDevCaps.szPname);
			}
		}
		if (id_default != nullptr) *id_default = 0;
		return s_list_devices;
	}


	AudioOut::ImplMME::ImplMME(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
		: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
	{
		m_samples_per_buffer = samplerate / 10;

		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.nSamplesPerSec = samplerate;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
		format.nBlockAlign = sizeof(short)*format.nChannels;
		format.wBitsPerSample = 16;
		format.cbSize = 0;

		waveOutOpen(&m_WaveOut, audio_device_id, &format, (DWORD_PTR)(SoundOutCallBack), (DWORD_PTR)this, CALLBACK_FUNCTION);

		m_Buffer = new short[m_samples_per_buffer * format.nChannels * 2];
		m_Buffer1 = m_Buffer;
		m_Buffer2 = m_Buffer + m_samples_per_buffer * format.nChannels;
		memset(m_Buffer, 0, sizeof(short)*m_samples_per_buffer * format.nChannels * 2);

		m_isPlaying = m_callback(m_Buffer1, m_samples_per_buffer, m_user_ptr);
		m_isPlaying = m_callback(m_Buffer2, m_samples_per_buffer, m_user_ptr);

		m_WaveHeader1.lpData = (char *)m_Buffer1;
		m_WaveHeader1.dwBufferLength = m_samples_per_buffer * format.nChannels * sizeof(short);
		m_WaveHeader1.dwFlags = 0;
		waveOutPrepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
		waveOutWrite(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));

		m_WaveHeader2.lpData = (char *)m_Buffer2;
		m_WaveHeader2.dwBufferLength = m_samples_per_buffer * format.nChannels * sizeof(short);
		m_WaveHeader2.dwFlags = 0;
		waveOutPrepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
		waveOutWrite(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
	}

	AudioOut::ImplMME::~ImplMME()
	{
		m_isPlaying = false;
		waveOutReset(m_WaveOut);
		waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
		waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
		waveOutClose(m_WaveOut);
		delete[] m_Buffer;
	}

	void AudioOut::ImplMME::SoundOutCallBack(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
	{
		if (uMsg == WOM_DONE)
		{
			AudioOut::ImplMME* self = (AudioOut::ImplMME*)dwInstance;
			WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
			if (!self->m_isPlaying)
			{
				self->m_eof_callback(self->m_user_ptr);
				return;
			}
			self->m_isPlaying = self->m_callback((short*)pwhr->lpData, self->m_samples_per_buffer, self->m_user_ptr);
			waveOutWrite(hwo, pwhr, sizeof(WAVEHDR));
		}
	}


	const std::vector<std::string>& AudioIn::ImplMME::s_get_list_audio_devices(int* id_default)
	{
		static std::vector<std::string> s_list_devices;
		if (s_list_devices.size() == 0)
		{
			unsigned num_dev = waveInGetNumDevs();
			WAVEINCAPS waveInDevCaps;
			for (unsigned i = 0; i < num_dev; i++)
			{
				waveInGetDevCaps(i, &waveInDevCaps, sizeof(WAVEINCAPS));
				s_list_devices.push_back(waveInDevCaps.szPname);
			}
		}
		if (id_default != nullptr) *id_default = 0;
		return s_list_devices;
	}

	AudioIn::ImplMME::ImplMME(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
		: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
	{
		m_samples_per_buffer = samplerate / 10;

		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.nSamplesPerSec = 44100;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
		format.nBlockAlign = sizeof(short) * format.nChannels;
		format.wBitsPerSample = 16;
		format.cbSize = 0;

		waveInOpen(&m_WaveIn, audio_device_id, &format, (DWORD_PTR)(s_SoundInCallback), (DWORD_PTR)this, CALLBACK_FUNCTION);

		m_Buffer = new short[m_samples_per_buffer * format.nChannels * 2];
		m_Buffer1 = m_Buffer;
		m_Buffer2 = m_Buffer + m_samples_per_buffer * format.nChannels;
		memset(m_Buffer, 0, sizeof(short)*m_samples_per_buffer * format.nChannels * 2);

		m_WaveHeader1.lpData = (char *)m_Buffer1;
		m_WaveHeader1.dwBufferLength = (DWORD)(m_samples_per_buffer * format.nChannels * sizeof(short));
		m_WaveHeader1.dwFlags = 0;
		waveInPrepareHeader(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));
		waveInAddBuffer(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));

		m_WaveHeader2.lpData = (char *)m_Buffer2;
		m_WaveHeader2.dwBufferLength = (DWORD)(m_samples_per_buffer* format.nChannels * sizeof(short));
		m_WaveHeader2.dwFlags = 0;
		waveInPrepareHeader(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));
		waveInAddBuffer(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));

		m_isReceiving = true;
		waveInStart(m_WaveIn);
	}

	AudioIn::ImplMME::~ImplMME()
	{
		m_isReceiving = false;
		waveInStop(m_WaveIn);
		waveInReset(m_WaveIn);
		waveInUnprepareHeader(m_WaveIn, &m_WaveHeader1, sizeof(WAVEHDR));
		waveInUnprepareHeader(m_WaveIn, &m_WaveHeader2, sizeof(WAVEHDR));
		waveInClose(m_WaveIn);

		delete[] m_Buffer;
	}

	void AudioIn::ImplMME::s_SoundInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
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
			self->m_isReceiving = self->m_callback((short*)pwhr->lpData, self->m_samples_per_buffer, self->m_user_ptr);
			waveInAddBuffer(hwi, pwhr, sizeof(WAVEHDR));
		}
	}
}
