#pragma once

#include "AudioCallbacks.h"
#include <memory>
#include <string>
#include <vector>

namespace LiveKit
{
	class AudioOut
	{
	public:
		AudioOut(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr);
		~AudioOut();

		class Implementer
		{
		public:
			Implementer() {}
			virtual ~Implementer() {}
		};

		class ImplPort;
		class ImplMME;
		class ImplWASAPI;

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:		
		std::unique_ptr<Implementer> m_impl;

	};

	class AudioIn
	{
	public:
		AudioIn(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr);
		~AudioIn();

		class Implementer
		{
		public:
			Implementer() {}
			virtual ~Implementer() {}
		};

		class ImplPort;
		class ImplMME;
		class ImplWASAPI;

		static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	private:
		std::unique_ptr<Implementer> m_impl;
	};
}