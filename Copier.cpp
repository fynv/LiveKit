#include "Copier.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

#include <thread>

namespace LiveKit
{
	Copier::Copier(const char* filename_in, const char* filename_out)
	{
		avformat_open_input(&m_p_fmt_ctx_in, filename_in, nullptr, nullptr);
		avformat_find_stream_info(m_p_fmt_ctx_in, nullptr);
		avformat_alloc_output_context2(&m_p_fmt_ctx_out, nullptr, nullptr, filename_out);

		size_t stream_mapping_size = m_p_fmt_ctx_in->nb_streams;
		m_stream_mapping.resize(stream_mapping_size);

		int stream_index = 0;
		for (unsigned i = 0; i < m_p_fmt_ctx_in->nb_streams; i++)
		{
			AVStream *in_stream = m_p_fmt_ctx_in->streams[i];
			AVCodecParameters *in_codecpar = in_stream->codecpar;

			if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
				in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
				m_stream_mapping[i] = -1;
				continue;
			}

			m_stream_mapping[i] = stream_index++;
			AVStream* out_stream = avformat_new_stream(m_p_fmt_ctx_out, nullptr);
			avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
			out_stream->codecpar->codec_tag = 0;
		}

		if (!(m_p_fmt_ctx_out->oformat->flags & AVFMT_NOFILE))
		{
			avio_open(&m_p_fmt_ctx_out->pb, filename_out, AVIO_FLAG_WRITE);
		}
		avformat_write_header(m_p_fmt_ctx_out, nullptr);

		m_copying = true;
		m_thread_copy = (std::unique_ptr<std::thread>)(new std::thread(thread_copy, this));
	}

	Copier::~Copier()
	{
		m_copying = false;
		m_thread_copy->join();
		m_thread_copy = nullptr;

		av_write_trailer(m_p_fmt_ctx_out);
		if (!(m_p_fmt_ctx_out->flags & AVFMT_NOFILE))
			avio_closep(&m_p_fmt_ctx_out->pb);
		avformat_free_context(m_p_fmt_ctx_out);

		avformat_close_input(&m_p_fmt_ctx_in);
	}

	void Copier::thread_copy(Copier* self)
	{
		AVPacket packet;
		while (self->m_copying && av_read_frame(self->m_p_fmt_ctx_in, &packet) == 0)
		{
			AVStream *in_stream = self->m_p_fmt_ctx_in->streams[packet.stream_index];
			if (packet.stream_index >= self->m_stream_mapping.size() ||
				self->m_stream_mapping[packet.stream_index] < 0) {
				av_packet_unref(&packet);
				continue;
			}
			packet.stream_index = self->m_stream_mapping[packet.stream_index];
			AVStream* out_stream = self->m_p_fmt_ctx_out->streams[packet.stream_index];

			packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
			packet.pos = -1;

			av_interleaved_write_frame(self->m_p_fmt_ctx_out, &packet);
			av_packet_unref(&packet);
		}

		self->m_copying = false;
	}

}

