#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace std
{
	class thread;
}

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct AVPacket;

namespace LiveKit
{
	class Image;
	class VideoTarget;
	class Camera
	{
	public:
		static const std::vector<std::string>* s_get_list_devices();

		Camera(int idx = 0);
		~Camera();

		int idx() const { return m_idx; }
		int width() const { return m_width; }
		int height() const { return m_height; }

		void AddTarget(VideoTarget* target)
		{
			m_targets.push_back(target);
		}

	private:
		int m_idx;
		int m_width, m_height;
		std::unique_ptr<Image> m_img;

		bool m_quit = false;
		int m_frame_rate_num, m_frame_rate_den;
		uint64_t m_start_time;
		size_t m_frame_count = 0;

		static void thread_read(Camera* self);
		std::unique_ptr<std::thread> m_thread_read;

		int m_v_idx;
		AVFormatContext* m_p_fmt_ctx;
		AVCodecContext* m_p_codec_ctx;
		AVFrame* m_p_frm_raw;
		AVFrame* m_p_frm_bgr;
		SwsContext* m_sws_ctx;
		std::unique_ptr<AVPacket> m_p_packet;

		std::vector<VideoTarget*> m_targets;
	};
}