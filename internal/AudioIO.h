#pragma once

#include "AudioPort.h"

namespace LiveKit
{
	class AudioOut
	{
	public:
		AudioOut(int audio_device_id, int samplerate, int samples_per_buffer, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr);
		~AudioOut();

	private:
		class Implementer;
		class ImplPort;
		class ImplMME;
		std::unique_ptr<Implementer> m_impl;
	};

	class AudioIn
	{
	public:
		AudioIn(int audio_device_id, int samplerate, int samples_per_buffer, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr);
		~AudioIn();

	private:
		class Implementer;
		class ImplPort;
		class ImplMME;
		std::unique_ptr<Implementer> m_impl;
	};

}

