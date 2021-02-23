#pragma once

#include <memory>

namespace LiveKit
{
	class AudioRepeater
	{
	public:
		AudioRepeater(int audio_device_id_in, int audio_device_id_out);
		~AudioRepeater();

	private:
		class AudioRecorder;
		class AudioPlayback;

		std::unique_ptr<AudioRecorder> m_audio_recorder;
		std::unique_ptr<AudioPlayback> m_audio_playback;

	};

}

