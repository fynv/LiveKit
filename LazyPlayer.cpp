#include "LazyPlayer.h"
#include "Image.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace LiveKit
{
	class LazyPlayer::Internal
	{
	public:
		Internal(const char* fn)
		{
			if (!exists_test(fn))
				printf("Failed loading %s\n", fn);

			avformat_open_input(&m_p_fmt_ctx, fn, nullptr, nullptr);
			avformat_find_stream_info(m_p_fmt_ctx, nullptr);
			m_duration = m_p_fmt_ctx->duration;

			for (unsigned i = 0; i < m_p_fmt_ctx->nb_streams; i++)
			{
				if (m_p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					m_v_idx = i;
					m_video_time_base_num = m_p_fmt_ctx->streams[i]->time_base.num;
					m_video_time_base_den = m_p_fmt_ctx->streams[i]->time_base.den;
				}
			}

			if (m_v_idx < 0)
			{
				printf("%s is not a video file.\n", fn);
			}

			AVCodecParameters* p_codec_par = m_p_fmt_ctx->streams[m_v_idx]->codecpar;
			AVCodec* p_codec = avcodec_find_decoder(p_codec_par->codec_id);
			m_p_codec_ctx_video = avcodec_alloc_context3(p_codec);
			avcodec_parameters_to_context(m_p_codec_ctx_video, p_codec_par);
			avcodec_open2(m_p_codec_ctx_video, p_codec, nullptr);

			m_p_frm_raw_video = av_frame_alloc();
			m_p_frm_bgr_video = av_frame_alloc();

			m_video_width = m_p_codec_ctx_video->width;
			m_video_height = m_p_codec_ctx_video->height;
			m_video_buffer = (std::unique_ptr<Image>)(new Image(m_video_width, m_video_height));

			av_image_fill_arrays(m_p_frm_bgr_video->data, m_p_frm_bgr_video->linesize, m_video_buffer->data(), AV_PIX_FMT_BGR24, m_video_width, m_video_height, 1);
			m_sws_ctx = sws_getContext(m_video_width, m_video_height, m_p_codec_ctx_video->pix_fmt, m_video_width, m_video_height, AV_PIX_FMT_BGR24, SWS_BICUBIC, nullptr, nullptr, nullptr);

			m_p_packet = std::unique_ptr<AVPacket>(new AVPacket);

			m_position = 0;
		}
		~Internal()
		{
			sws_freeContext(m_sws_ctx);
			av_frame_free(&m_p_frm_bgr_video);
			av_frame_free(&m_p_frm_raw_video);
			avcodec_free_context(&m_p_codec_ctx_video);
			avformat_close_input(&m_p_fmt_ctx);
		}

		int width() const { return m_video_width; }
		int height() const { return m_video_height; }
		uint64_t get_duration() const { return m_duration; }
		uint64_t get_position() const { return m_position; }
		void set_position(uint64_t pos)
		{
			m_position = pos;
			avformat_seek_file(m_p_fmt_ctx, -1, INT64_MIN, pos, INT64_MAX, 0);
		}

		bool fetch()
		{
			bool frame_read = false;
			while (av_read_frame(m_p_fmt_ctx, m_p_packet.get()) == 0)
			{
				if (m_p_packet->stream_index == m_v_idx)
				{
					int64_t t = m_p_packet->dts * m_video_time_base_num * AV_TIME_BASE / m_video_time_base_den;
					avcodec_send_packet(m_p_codec_ctx_video, m_p_packet.get());
					avcodec_receive_frame(m_p_codec_ctx_video, m_p_frm_raw_video);
					av_packet_unref(m_p_packet.get());
					if (t >= (int64_t)m_position)
					{
						m_position = (uint64_t)t;
						sws_scale(m_sws_ctx, (const uint8_t *const *)m_p_frm_raw_video->data, m_p_frm_raw_video->linesize,
							0, m_p_codec_ctx_video->height, m_p_frm_bgr_video->data, m_p_frm_bgr_video->linesize);
						frame_read = true;
						break;
					}
				}
				else
				{
					av_packet_unref(m_p_packet.get());
				}
			}
			return frame_read;
		}

		const Image* get_image() const
		{
			return m_video_buffer.get();
		}

	private:

		AVFormatContext* m_p_fmt_ctx = nullptr;
		int m_v_idx = -1;
		int m_video_time_base_num, m_video_time_base_den;
		int m_video_width, m_video_height;
		uint64_t m_duration;
		uint64_t m_position;

		AVCodecContext* m_p_codec_ctx_video;
		AVFrame *m_p_frm_raw_video;
		AVFrame *m_p_frm_bgr_video;
		std::unique_ptr<Image> m_video_buffer;
		SwsContext* m_sws_ctx;

		std::unique_ptr<AVPacket> m_p_packet;

	};

	LazyPlayer::LazyPlayer(const char* fn) : m_internal(new Internal(fn))
	{
		
	}

	LazyPlayer::~LazyPlayer()
	{

	}

	int LazyPlayer::video_width() const
	{
		return m_internal->width();
	}

	int LazyPlayer::video_height() const
	{
		return m_internal->height();
	}

	bool LazyPlayer::is_eof_reached() const
	{
		uint64_t duration = m_internal->get_duration();
		uint64_t cur_pos = get_position();
		return cur_pos >= duration;
	}

	uint64_t LazyPlayer::get_duration() const
	{
		return m_internal->get_duration();
	}

	uint64_t LazyPlayer::get_position() const
	{
		if (m_is_playing)
		{
			return m_start_pos + (time_micro_sec() - m_start_time);
		}
		else
		{
			return m_start_pos;
		}
	}

	void LazyPlayer::stop()
	{
		if (m_is_playing)
		{
			m_start_pos = get_position();
			m_start_time = time_micro_sec();
			m_is_playing = false;
		}
	}

	void LazyPlayer::start()
	{
		if (!m_is_playing)
		{
			m_start_time = time_micro_sec();
			m_is_playing = true;
		}
	}

	void LazyPlayer::set_position(uint64_t pos)
	{
		m_start_pos = pos;
		m_start_time = time_micro_sec();
		m_internal->set_position(pos);
		if (m_internal->fetch())
		{
			m_timestamp = time_micro_sec();
		}
	}

	const Image* LazyPlayer::read_image(uint64_t* timestamp) const
	{
		uint64_t duration = m_internal->get_duration();
		uint64_t cur_pos = get_position();
		if (cur_pos >= duration)
		{
			*timestamp = m_timestamp;
			return m_internal->get_image();
		}

		while (m_internal->get_position() < cur_pos)
		{
			if (!m_internal->fetch()) break;
			m_timestamp = time_micro_sec();
		}
		*timestamp = m_timestamp;
		return m_internal->get_image();
	}
}
