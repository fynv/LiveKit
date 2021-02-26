#pragma once

#include "AudioIO.h"

#include <string>
#include <vector>
#include <mmdeviceapi.h>
#include <Audioclient.h>

namespace std
{
	class thread;
}

namespace LiveKit
{
	class AudioOut::ImplWASAPI : public Implementer
	{
	public:
		ImplWASAPI(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr);
		~ImplWASAPI();

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:
		AudioWriteCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;

		IMMDevice *m_pDevice;
		IAudioClient *m_pAudioClient;
		HANDLE m_hEvent;
		UINT32 m_bufferFrameCount;
		IAudioRenderClient *m_pRenderClient = nullptr;

		bool m_playing = false;
		static void thread_play(ImplWASAPI* self);
		std::unique_ptr<std::thread> m_thread_play;
	};

	class AudioIn::ImplWASAPI : public Implementer
	{
	public:
		ImplWASAPI(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr);
		~ImplWASAPI();

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:
		AudioReadCallback m_callback;
		EOFCallback m_eof_callback;
		void* m_user_ptr;

		IMMDevice *m_pDevice;
		IAudioClient *m_pAudioClient;
		HANDLE m_hEvent;
		UINT32 m_bufferFrameCount;
		IAudioCaptureClient *m_pCaptureClient = nullptr;

		bool m_recording = false;
		static void thread_record(ImplWASAPI* self);
		std::unique_ptr<std::thread> m_thread_record;
	};

}