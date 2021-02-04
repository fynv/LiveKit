#include "Image.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


namespace LiveKit
{
	Image::Image(int width, int height, bool has_alpha)
	{
		m_has_alpha = has_alpha;
		m_width = width;
		m_height = height;
		AVPixelFormat out_pix_fmt = m_has_alpha ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGR24;
		int buf_size = av_image_get_buffer_size(out_pix_fmt, m_width, m_height, 1);
		m_buffer = (uint8_t *)av_malloc(buf_size);
	}

	Image::Image(const char* fn, bool keep_alpha)
	{
		if (!exists_test(fn))
			printf("Failed loading %s\n", fn);

		AVFormatContext* p_fmt_ctx = nullptr;
		avformat_open_input(&p_fmt_ctx, fn, nullptr, nullptr);
		avformat_find_stream_info(p_fmt_ctx, nullptr);

		int v_idx = -1;
		int frame_rate;
		for (unsigned i = 0; i < p_fmt_ctx->nb_streams; i++)
		{
			if (p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				v_idx = i;
				frame_rate = p_fmt_ctx->streams[i]->avg_frame_rate.num / p_fmt_ctx->streams[i]->avg_frame_rate.den;
				break;
			}
		}

		AVCodecParameters* p_codec_par = p_fmt_ctx->streams[v_idx]->codecpar;
		AVCodec* p_codec = avcodec_find_decoder(p_codec_par->codec_id);
		AVCodecContext* p_codec_ctx = avcodec_alloc_context3(p_codec);
		avcodec_parameters_to_context(p_codec_ctx, p_codec_par);
		avcodec_open2(p_codec_ctx, p_codec, NULL);

		const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(p_codec_ctx->pix_fmt);
		m_has_alpha = keep_alpha && (desc->flags& AV_PIX_FMT_FLAG_ALPHA != 0);
		m_width = p_codec_ctx->width;
		m_height = p_codec_ctx->height;
		AVPixelFormat out_pix_fmt = m_has_alpha ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGR24;

		AVFrame* p_frm_raw = av_frame_alloc();
		AVFrame* p_frm_bgr = av_frame_alloc();

		int buf_size = av_image_get_buffer_size(out_pix_fmt, m_width, m_height, 1);
		m_buffer = (uint8_t *)av_malloc(buf_size);

		av_image_fill_arrays(p_frm_bgr->data, p_frm_bgr->linesize, m_buffer, out_pix_fmt, m_width, m_height, 1);
		SwsContext* sws_ctx = sws_getContext(m_width, m_height, p_codec_ctx->pix_fmt, m_width, m_height, out_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

		AVPacket packet;
		while (true)
		{
			av_read_frame(p_fmt_ctx, &packet);
			if (packet.stream_index == v_idx) break;
			else
			{
				av_packet_unref(&packet);
			}
		}

		avcodec_send_packet(p_codec_ctx, &packet);
		avcodec_receive_frame(p_codec_ctx, p_frm_raw);
		av_packet_unref(&packet);

		sws_scale(sws_ctx, (const uint8_t *const *)p_frm_raw->data, p_frm_raw->linesize, 0, p_codec_ctx->height, p_frm_bgr->data, p_frm_bgr->linesize);

		sws_freeContext(sws_ctx);	
		av_frame_free(&p_frm_bgr);
		av_frame_free(&p_frm_raw);
		avcodec_free_context(&p_codec_ctx);
		avformat_close_input(&p_fmt_ctx);
	}


	Image::Image(const Image& in)
	{
		m_has_alpha = in.m_has_alpha;
		m_width = in.m_width;
		m_height = in.m_height;
		AVPixelFormat out_pix_fmt = m_has_alpha ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGR24;
		int buf_size = av_image_get_buffer_size(out_pix_fmt, m_width, m_height, 1);
		m_buffer = (uint8_t *)av_malloc(buf_size);
		memcpy(m_buffer, in.m_buffer, buf_size);
		m_flipped = in.m_flipped;
	}

	Image::~Image()
	{
		av_free(m_buffer);
	}

	const uint8_t* Image::get_data(int& width, int& height) const
	{
		width = m_width;
		height = m_height;
		return m_buffer;
	}

	const Image& Image::operator=(const Image& in)
	{
		AVPixelFormat out_pix_fmt = m_has_alpha ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGR24;
		int buf_size = av_image_get_buffer_size(out_pix_fmt, m_width, m_height, 1);
		memcpy(m_buffer, in.m_buffer, buf_size);
		m_flipped = in.m_flipped;
		return *this;
	}
}

