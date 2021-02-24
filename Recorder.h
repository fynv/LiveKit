#pragma once

#include <memory>


namespace std
{
	class thread;
}

struct AVFormatContext;
struct AVOutputFormat;

namespace LiveKit
{
	class Image;
	class VideoSource;
	struct OutputStream;

	class Recorder
	{
	public:
		Recorder(const char* filename, bool mp4, int video_width, int video_height, bool record_audio = false, int audio_device_id = 0);
		~Recorder();

		void SetSource(const VideoSource* source)
		{
			m_source = source;
		}

		void start();
		void stop();


	private:	
		class AudioRecorder;		

		int m_video_width, m_video_height;
		bool m_record_audio;
		int m_audio_device_id;

		AVFormatContext *m_oc;
		AVOutputFormat *m_fmt;
		std::unique_ptr<OutputStream> m_video_st;
		std::unique_ptr<OutputStream> m_audio_st;

		std::unique_ptr<AudioRecorder> m_audio_recorder;
		const VideoSource* m_source = nullptr;

		bool m_recording = false;
		uint64_t m_start_time;
		size_t m_frame_count;
		void update_video();
		void update_audio();
		void update_av();
		static void thread_write(Recorder* self);
		std::unique_ptr<std::thread> m_thread_write;

	};

}