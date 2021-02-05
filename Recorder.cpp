#include "Recorder.h"
#include "Image.h"
#include "VideoPort.h"
#include "BufferQueue.h"
#include "SoundRecorder.h"
#include "Utils.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <thread>

namespace LiveKit
{
	static const AVCodecID video_codec_id = AV_CODEC_ID_H264;
	static const AVCodecID audio_codec_id = AV_CODEC_ID_AAC;
	static const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
	static const AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;
	static const int fps = 30;
	static const int sample_rate = 44100;

	struct OutputStream
	{
		AVStream *st = nullptr;
		AVCodecContext *enc = nullptr;

		/* pts of the next frame that will be generated */
		int64_t next_pts = 0;
		int samples_count = 0;

		AVFrame *frame = nullptr;
		AVFrame *tmp_frame = nullptr;
		unsigned char *tmp_buffer = nullptr;

		struct SwsContext *sws_ctx = nullptr;
		struct SwrContext *swr_ctx = nullptr;
	};


	inline void add_video_stream(OutputStream *ost, AVFormatContext *oc, AVCodec *codec, int width, int height)
	{
		ost->st = avformat_new_stream(oc, NULL);
		ost->st->id = oc->nb_streams - 1;
		AVCodecContext *c = avcodec_alloc_context3(codec);
		ost->enc = c;
		c->codec_id = video_codec_id;
		c->bit_rate = width * height * 3;
		c->width = width;
		c->height = height;
		ost->st->time_base = { 1, fps };
		c->time_base = ost->st->time_base;
		c->gop_size = 15;
		c->pix_fmt = pix_fmt;
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	inline void add_audio_stream(OutputStream *ost, AVFormatContext *oc, AVCodec *codec)
	{
		ost->st = avformat_new_stream(oc, NULL);
		ost->st->id = oc->nb_streams - 1;
		AVCodecContext *c = avcodec_alloc_context3(codec);
		ost->enc = c;
		c->sample_fmt = AV_SAMPLE_FMT_FLTP;
		c->bit_rate = 128000;
		c->sample_rate = sample_rate;
		c->channel_layout = AV_CH_LAYOUT_STEREO;
		c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
		ost->st->time_base = { 1, c->sample_rate };
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	inline AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
	{
		AVFrame *picture = av_frame_alloc();
		picture->format = pix_fmt;
		picture->width = width;
		picture->height = height;
		av_frame_get_buffer(picture, 0);
		return picture;
	}


	inline void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
	{
		AVCodecContext *c = ost->enc;
		/* open the codec */
		avcodec_open2(c, codec, nullptr);
		/* allocate and init a re-usable frame */
		ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
		avcodec_parameters_from_context(ost->st->codecpar, c);
		ost->sws_ctx = sws_getContext(c->width, c->height, AV_PIX_FMT_BGR24, c->width, c->height, c->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
	}

	inline AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
	{
		AVFrame *frame = av_frame_alloc();
		frame->format = sample_fmt;
		frame->channel_layout = channel_layout;
		frame->sample_rate = sample_rate;
		frame->nb_samples = nb_samples;
		av_frame_get_buffer(frame, 0);
		return frame;
	}


	inline void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost)
	{
		AVCodecContext *c;
		c = ost->enc;

		/* open it */
		avcodec_open2(c, codec, nullptr);
		int nb_samples = c->frame_size;

		ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
		ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout, c->sample_rate, nb_samples);

		avcodec_parameters_from_context(ost->st->codecpar, c);
		ost->swr_ctx = swr_alloc();

		/* set options */
		av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
		av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
		av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

		swr_init(ost->swr_ctx);
	}


	inline void close_stream(AVFormatContext *oc, OutputStream *ost)
	{
		avcodec_free_context(&ost->enc);
		av_frame_free(&ost->frame);
		av_frame_free(&ost->tmp_frame);
		if (ost->tmp_buffer != nullptr) free(ost->tmp_buffer);
		sws_freeContext(ost->sws_ctx);
		swr_free(&ost->swr_ctx);
	}

	Recorder::Recorder(const char* filename, bool mp4, int video_width, int video_height, int audio_device_id)
		: m_video_width(video_width), m_video_height(video_height), m_audio_device_id(audio_device_id)
	{
		AVOutputFormat *output_format = av_guess_format(mp4?"mp4":"flv", nullptr, filename);
		avformat_alloc_output_context2(&m_oc, output_format, nullptr, filename);		
		m_fmt = m_oc->oformat;

		AVCodec *video_codec = avcodec_find_encoder(video_codec_id);
		m_video_st = (std::unique_ptr<OutputStream>)(new OutputStream);
		add_video_stream(m_video_st.get(), m_oc, video_codec, m_video_width, m_video_height);
		open_video(m_oc, video_codec, m_video_st.get());

		if (m_audio_device_id >= 0)
		{
			AVCodec *audio_codec = avcodec_find_encoder(audio_codec_id);
			m_audio_st = (std::unique_ptr<OutputStream>)(new OutputStream);
			add_audio_stream(m_audio_st.get(), m_oc, audio_codec);
			open_audio(m_oc, audio_codec, m_audio_st.get());
		}

		av_dump_format(m_oc, 0, filename, 1);
		if (!(m_fmt->flags & AVFMT_NOFILE))
		{
			avio_open(&m_oc->pb, filename, AVIO_FLAG_WRITE);
		}
		avformat_write_header(m_oc, nullptr);
	}

	Recorder::~Recorder()
	{
		stop();
		av_write_trailer(m_oc);
		close_stream(m_oc, m_video_st.get());
		if (m_audio_st!=nullptr)
			close_stream(m_oc, m_audio_st.get());
		if (!(m_fmt->flags & AVFMT_NOFILE))
			avio_closep(&m_oc->pb);
		avformat_free_context(m_oc);
	}

	void Recorder::start()
	{
		if (!m_recording)
		{
			if (m_audio_device_id >= 0)
			{
				int nb_samples = m_audio_st->enc->frame_size;
				m_sound_recorder = (std::unique_ptr<SoundRecorder>)(new SoundRecorder(m_audio_device_id, nb_samples));
			}
			m_recording = true;
			m_start_time = time_micro_sec();
			m_frame_count = 0;
			m_thread_write = (std::unique_ptr<std::thread>)(new std::thread(thread_write, this));
		}
	}

	void Recorder::stop()
	{
		if (m_recording)
		{
			m_recording = false;
			m_thread_write->join();
			m_thread_write = nullptr;
			m_sound_recorder = nullptr;
		}
	}



	inline int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame)
	{
		int ret;
		ret = avcodec_send_frame(c, frame);

		while (ret >= 0)
		{
			AVPacket pkt = { 0 };

			ret = avcodec_receive_packet(c, &pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
			pkt.stream_index = st->index;
			av_interleaved_write_frame(fmt_ctx, &pkt);
			av_packet_unref(&pkt);
		}

		return ret == AVERROR_EOF ? 1 : 0;
	}	

	void Recorder::update_video()
	{
		AVCodecContext *c = m_video_st->enc;
		if (m_video_st->tmp_buffer == nullptr)
			m_video_st->tmp_buffer = (uint8_t*)malloc((size_t)m_video_width* m_video_height * 3);

		memset(m_video_st->tmp_buffer, 0, (size_t)m_video_width*m_video_height * 3);
		bool flipped = false;
		if (m_source != nullptr)
		{
			uint64_t timestamp;
			const Image* img_in = m_source->read_image(&timestamp);
			int width_in = img_in->width();
			int height_in = img_in->height();
			int chn_in = img_in->has_alpha() ? 4 : 3;

			int offset_in_x = 0;
			int offset_in_y = 0;
			int offset_out_x = 0;
			int offset_out_y = 0;
			int scan_w = min(m_video_width, width_in);
			int scan_h = min(m_video_height, height_in);

			if (width_in < m_video_width)
				offset_out_x = (m_video_width - width_in) / 2;
			else if (width_in > m_video_width)
				offset_in_x = (width_in - m_video_width) / 2;

			if (height_in < m_video_height)
				offset_out_y = (m_video_height - height_in) / 2;
			else if (height_in > m_video_height)
				offset_in_y = (height_in - m_video_height) / 2;

			for (int y = 0; y < scan_h; y++)
			{
				const uint8_t* p_in_line = img_in->data() + (y+ offset_in_y)*width_in*chn_in;
				uint8_t* p_out_line = m_video_st->tmp_buffer + (y + offset_out_y)*m_video_width*3;
				for (int x = 0; x < scan_w; x++)
				{
					const uint8_t* p_in = p_in_line + (x + offset_in_x)*chn_in;
					uint8_t* p_out = p_out_line + (x + offset_out_x)*3;
					p_out[0] = p_in[0];
					p_out[1] = p_in[1];
					p_out[2] = p_in[2];
				}
			}

			flipped = img_in->is_flipped();
		}

		av_frame_make_writable(m_video_st->frame);
		const unsigned char* p_data = m_video_st->tmp_buffer;
		int stride = c->width * 3;		
		if (flipped)
		{
			p_data += stride * (c->height - 1);
			stride = -stride;
		}
		sws_scale(m_video_st->sws_ctx, &p_data, &stride, 0, c->height, m_video_st->frame->data, m_video_st->frame->linesize);

		m_video_st->frame->pts = m_video_st->next_pts++;
		write_frame(m_oc, m_video_st->enc, m_video_st->st, m_video_st->frame);
		
	}

	void Recorder::update_audio()
	{
		Buffer* buf = m_sound_recorder->get_buffer();

		AVCodecContext *c = m_audio_st->enc;
		int16_t *q = (int16_t*)m_audio_st->tmp_frame->data[0];
		memcpy(q, buf->m_data, sizeof(short)*m_audio_st->tmp_frame->nb_samples*2);
		m_sound_recorder->recycle_buffer(buf);

		m_audio_st->tmp_frame->pts = m_audio_st->next_pts;
		m_audio_st->next_pts += m_audio_st->tmp_frame->nb_samples;

		int dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(m_audio_st->swr_ctx, c->sample_rate) + m_audio_st->tmp_frame->nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);

		av_frame_make_writable(m_audio_st->frame);

		swr_convert(m_audio_st->swr_ctx, m_audio_st->frame->data, dst_nb_samples, (const uint8_t **)m_audio_st->tmp_frame->data, m_audio_st->tmp_frame->nb_samples);

		m_audio_st->frame->pts = av_rescale_q(m_audio_st->samples_count, { 1, c->sample_rate }, c->time_base);
		m_audio_st->samples_count += dst_nb_samples;

		write_frame(m_oc, c, m_audio_st->st, m_audio_st->frame);
	}

	void Recorder::update_av()
	{
		if (av_compare_ts(m_video_st->next_pts, m_video_st->enc->time_base, m_audio_st->next_pts, m_audio_st->enc->time_base) <= 0)
		{
			update_video();
		}
		else
		{
			update_audio();
		}
	}

	void Recorder::thread_write(Recorder* self)
	{
		while (self->m_recording)
		{
			if (self->m_audio_device_id >= 0)
			{
				self->update_av();
			}
			else
			{
				self->m_frame_count++;
				uint64_t target = self->m_start_time + self->m_frame_count * AV_TIME_BASE / fps;
				uint64_t now = time_micro_sec();
				int64_t delta = target - now;
				if (delta > 0)
					std::this_thread::sleep_for(std::chrono::microseconds(delta));

				self->update_video();
			}

		}
	}
}