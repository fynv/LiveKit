#include "Recorder.h"
#include "Image.h"
#include "VideoPort.h"
#include "AudioIO.h"
#include "BufferQueue.h"
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
#include <cmath>
#include <queue>

namespace LiveKit
{
	class Recorder::AudioRecorder
	{
	public:
		AudioRecorder(int devId)
		{
			m_audio_in = (std::unique_ptr<AudioIn>)(new AudioIn(devId, 44100, callback, eof_callback, this));
		}

		~AudioRecorder()
		{
			m_audio_in = nullptr;
		}

		AudioBuffer* get_buffer()
		{
			return m_buffer_queue.PopBuffer();
		}

	private:
		BufferQueue m_buffer_queue;
		std::unique_ptr<AudioIn> m_audio_in;

		static void eof_callback(void* usr_ptr) {}

		static bool callback(const short* buf, size_t buf_size, void* usr_ptr)
		{
			AudioRecorder* self = (AudioRecorder*)usr_ptr;
			self->recordBuf(buf, buf_size);
			return true;
		}

		void recordBuf(const short* buf, size_t buf_size)
		{
			AudioBuffer* newBuf = new AudioBuffer(2, buf_size);
			memcpy(newBuf->data(), buf, buf_size * sizeof(short) * 2);
			m_buffer_queue.PushBuffer(newBuf);
		}
	};

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

		static double a = 0.078;
		static double b = 0.2027;
		double mega_pixels = (double)(width * height) / 1000000.0;
		double mega_bps = (sqrt(b*b + 4 * a*mega_pixels) - b) / (2 * a);
		c->bit_rate = (int64_t)(mega_bps*1000000.0);
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

	Recorder::Recorder(const char* filename, bool mp4, int video_width, int video_height, bool record_audio, int audio_device_id)
		: m_video_width(video_width), m_video_height(video_height), m_record_audio(record_audio), m_audio_device_id(audio_device_id)
	{
		AVOutputFormat *output_format = av_guess_format(mp4 ? "mp4" : "flv", nullptr, filename);
		avformat_alloc_output_context2(&m_oc, output_format, nullptr, filename);
		m_fmt = m_oc->oformat;

		AVCodec *video_codec = avcodec_find_encoder(video_codec_id);
		m_video_st = (std::unique_ptr<OutputStream>)(new OutputStream);
		add_video_stream(m_video_st.get(), m_oc, video_codec, m_video_width, m_video_height);
		open_video(m_oc, video_codec, m_video_st.get());

		if (record_audio)
		{
			AVCodec *audio_codec = avcodec_find_encoder(audio_codec_id);
			m_audio_st = (std::unique_ptr<OutputStream>)(new OutputStream);
			add_audio_stream(m_audio_st.get(), m_oc, audio_codec);
			open_audio(m_oc, audio_codec, m_audio_st.get());
		}

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
		if (m_audio_st != nullptr)
			close_stream(m_oc, m_audio_st.get());
		if (!(m_fmt->flags & AVFMT_NOFILE))
			avio_closep(&m_oc->pb);
		avformat_free_context(m_oc);
	}

	void Recorder::start()
	{
		if (!m_recording)
		{
			if (m_record_audio)
				m_audio_recorder = (std::unique_ptr<AudioRecorder>)(new AudioRecorder(m_audio_device_id));

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
			m_audio_recorder = nullptr;
			delete m_buf_in;
			m_buf_in = nullptr;
			m_in_pos = 0;
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

	void Recorder::update_audio()
	{
		AVCodecContext *c = m_audio_st->enc;
		int16_t *buf = (int16_t*)m_audio_st->tmp_frame->data[0];

		int out_len = m_audio_st->tmp_frame->nb_samples;
		int out_pos = 0;
		while (out_pos < out_len)
		{
			if (m_buf_in != nullptr && m_in_pos < m_buf_in->len())
			{
				int copy_size = m_buf_in->len() - m_in_pos;
				int copy_size_out = out_len - out_pos;
				if (copy_size > copy_size_out) copy_size = copy_size_out;
				short* p_out = buf + out_pos * 2;
				const short* p_in = (const short*)m_buf_in->data() + m_in_pos * 2;
				memcpy(p_out, p_in, sizeof(short)*copy_size * 2);
				out_pos += copy_size;
				m_in_pos += copy_size;
			}
			else
			{
				delete m_buf_in;
				m_buf_in = m_audio_recorder->get_buffer();
				m_in_pos = 0;
			}
		}

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

	void Recorder::update_video()
	{
		AVCodecContext *c = m_video_st->enc;
		if (m_video_st->tmp_buffer == nullptr)
			m_video_st->tmp_buffer = (uint8_t*)malloc((size_t)m_video_width* m_video_height * 3);

		memset(m_video_st->tmp_buffer, 0, (size_t)m_video_width*m_video_height * 3);
		if (m_source != nullptr)
		{
			uint64_t timestamp;
			const Image* img_in = m_source->read_image(&timestamp);
			if (timestamp != (uint64_t)(-1))
			{
				copy_centered(img_in->data(), img_in->width(), img_in->height(), img_in->has_alpha() ? 4 : 3,
					m_video_st->tmp_buffer, m_video_width, m_video_height, 3, img_in->is_flipped());
			}
		}

		av_frame_make_writable(m_video_st->frame);
		const unsigned char* p_data = m_video_st->tmp_buffer;
		int stride = c->width * 3;
		sws_scale(m_video_st->sws_ctx, &p_data, &stride, 0, c->height, m_video_st->frame->data, m_video_st->frame->linesize);

		m_video_st->frame->pts = m_video_st->next_pts++;
		write_frame(m_oc, m_video_st->enc, m_video_st->st, m_video_st->frame);

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
			if (self->m_record_audio)
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
