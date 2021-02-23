#pragma once
#include <cstdint>
#include <memory>
#include <Windows.h>
#include <vector>

namespace std
{
	class thread;
}

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwrContext;
struct SwsContext;
struct AVPacket;

namespace LiveKit
{
	struct MediaInfo
	{
		uint64_t duration = 0;
		bool has_video = false;
		int video_width = 0;
		int video_height = 0;
		double video_fps = 0;
		int video_bitrate = 0;
		bool has_audio = false;
		int audio_sample_rate = 0;
		int audio_number_of_channels = 0;
		int audio_bitrate = 0;
	};

	void get_media_info(const char* fn, MediaInfo* info);

	class AudioBuffer;
	class Image;
	class VideoTarget;

	class Player
	{
	public:
		Player(const char* fn, bool play_audio = true, bool play_video = true, int audio_device_id = 0);
		~Player();

		void AddTarget(VideoTarget* target)
		{
			m_targets.push_back(target);
		}

		int video_width() const { return m_video_width; }
		int video_height() const { return m_video_height; }

		bool is_playing() const;
		bool is_eof_reached() const;
		uint64_t get_duration() const { return m_duration; }
		uint64_t get_position() const;		

		void stop();
		void start();
		void set_position(uint64_t pos);
		void set_audio_device(int audio_device_id);


	private:
		class PacketQueue;
		class AudioPlayback;
		class VideoPlayback;

		void _start(uint64_t pos);

		std::unique_ptr<PacketQueue> m_queue_audio;
		std::unique_ptr<PacketQueue> m_queue_video;

		AVFormatContext* m_p_fmt_ctx = nullptr;

		int m_a_idx = -1;
		int m_v_idx = -1;

		int m_audio_time_base_num, m_audio_time_base_den;
		int m_video_time_base_num, m_video_time_base_den;
		int m_video_width, m_video_height;
		uint64_t m_duration;

		AVCodecContext* m_p_codec_ctx_audio;
		AVFrame *m_p_frm_raw_audio;
		AVFrame *m_p_frm_s16_audio;
		std::unique_ptr<AudioBuffer> m_audio_buffer;
		SwrContext *m_swr_ctx;

		AVCodecContext* m_p_codec_ctx_video;
		AVFrame *m_p_frm_raw_video;
		AVFrame *m_p_frm_bgr_video;
		std::unique_ptr<Image> m_video_buffer;
		SwsContext* m_sws_ctx;

		std::unique_ptr<AVPacket> m_p_packet;
		bool m_demuxing = false;
		bool m_audio_playing = false;
		bool m_video_playing = false;
		bool m_audio_eof = true;
		bool m_video_eof = true;

		uint64_t m_sync_local_time;
		uint64_t m_sync_progress;
		mutable CRITICAL_SECTION m_cs_sync;
		void _set_sync_point(uint64_t local_time, uint64_t progress);
		void _get_sync_point(uint64_t& local_time, uint64_t& progress) const;

		int m_audio_device_id;

		static void thread_demux(Player* self);
		std::unique_ptr<std::thread> m_thread_demux;

		std::unique_ptr<AudioPlayback> m_audio_playback;
		std::unique_ptr<VideoPlayback> m_video_playback;

		std::vector<VideoTarget*> m_targets;

	};



}
