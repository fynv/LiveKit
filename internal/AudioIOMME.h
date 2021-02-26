#pragma once

#include "AudioIO.h"

#include <Windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <string>
#include <vector>

namespace LiveKit
{
	class AudioOut::ImplMME : public Implementer
	{
	public:
		ImplMME(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr);
		~ImplMME();

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:
		AudioWriteCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;

		int m_samples_per_buffer;
		bool m_isPlaying = true;
		HWAVEOUT m_WaveOut;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR	m_WaveHeader1, m_WaveHeader2;

		static void CALLBACK SoundOutCallBack(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	};

	class AudioIn::ImplMME : public Implementer
	{
	public:
		ImplMME(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr);
		~ImplMME();

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:
		AudioReadCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;

		int m_samples_per_buffer;
		bool m_isReceiving = false;
		HWAVEIN m_WaveIn;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR m_WaveHeader1, m_WaveHeader2;

		static void CALLBACK s_SoundInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

	};

}

