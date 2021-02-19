#include "Player.h"
#include "AudioBuffer.h"
#include "Image.h"
#include "VideoPort.h"
#include "Utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <mmsystem.h>
#include <mmreg.h>
#include <queue>
#include <thread>

namespace LiveKit
{
	void get_media_info(const char* fn, MediaInfo* info)
	{
		AVFormatContext* p_fmt_ctx = nullptr;
		avformat_open_input(&p_fmt_ctx, fn, nullptr, nullptr);
		avformat_find_stream_info(p_fmt_ctx, nullptr);
		info->duration = p_fmt_ctx->duration;
		for (unsigned i = 0; i < p_fmt_ctx->nb_streams; i++)
		{
			if (p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				info->has_audio = true;
				info->audio_sample_rate = p_fmt_ctx->streams[i]->codecpar->sample_rate;
				info->audio_number_of_channels = p_fmt_ctx->streams[i]->codecpar->channels;
				info->audio_bitrate = p_fmt_ctx->streams[i]->codecpar->bit_rate;
			}
			if (p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				info->has_video = true;
				int fps_num = p_fmt_ctx->streams[i]->avg_frame_rate.num;
				int fps_den = p_fmt_ctx->streams[i]->avg_frame_rate.den;
				info->video_fps = (double)fps_num / (double)fps_den;
				info->video_width = p_fmt_ctx->streams[i]->codecpar->width;
				info->video_height = p_fmt_ctx->streams[i]->codecpar->height;
				info->video_bitrate = p_fmt_ctx->streams[i]->codecpar->bit_rate;
			}
		}
		avformat_close_input(&p_fmt_ctx);
	}


	class Player::PacketQueue
	{
	public:
		PacketQueue(int cache_size)
		{
			InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
			m_semaphore_in = CreateSemaphore(nullptr, cache_size, (1u << 31) - 1, nullptr);
			m_semaphore_out = CreateSemaphore(nullptr, 0, (1u << 31) - 1, nullptr);
		}

		~PacketQueue()
		{
			CloseHandle(m_semaphore_out);
			CloseHandle(m_semaphore_in);
			DeleteCriticalSection(&m_cs);
		}

		size_t Size()
		{
			return m_queue.size();
		}

		AVPacket Front()
		{
			return m_queue.front();
		}

		void Push(AVPacket packet)
		{
			WaitForSingleObject(m_semaphore_in, INFINITE);
			EnterCriticalSection(&m_cs);
			m_queue.push(packet);
			LeaveCriticalSection(&m_cs);
			ReleaseSemaphore(m_semaphore_out, 1, nullptr);
		}

		AVPacket Pop()
		{
			WaitForSingleObject(m_semaphore_out, INFINITE);
			EnterCriticalSection(&m_cs);
			AVPacket ret = m_queue.front();
			m_queue.pop();
			LeaveCriticalSection(&m_cs);
			ReleaseSemaphore(m_semaphore_in, 1, nullptr);
			return ret;
		}

	private:
		std::queue<AVPacket> m_queue;
		CRITICAL_SECTION m_cs;
		HANDLE m_semaphore_in;
		HANDLE m_semaphore_out;
	};

#define DEV_SAMPLES_PER_BUFFER 4096

	class Player::AudioPlayback
	{
	public:
		AudioPlayback(int audioDevId, Player* player) : m_player(player)
		{
			WAVEFORMATEX format = {};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = 2;
			format.nSamplesPerSec = player->m_p_codec_ctx_audio->sample_rate;
			format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
			format.nBlockAlign = sizeof(short)*format.nChannels;
			format.wBitsPerSample = 16;
			format.cbSize = 0;

			waveOutOpen(&m_WaveOut, audioDevId, &format, (DWORD_PTR)(SoundOutCallBack), (DWORD_PTR)this, CALLBACK_FUNCTION);

			m_Buffer = new short[DEV_SAMPLES_PER_BUFFER * format.nChannels * 2];
			m_Buffer1 = m_Buffer;
			m_Buffer2 = m_Buffer + DEV_SAMPLES_PER_BUFFER * format.nChannels;
			memset(m_Buffer, 0, sizeof(short)*DEV_SAMPLES_PER_BUFFER * format.nChannels * 2);

			m_WaveHeader1.lpData = (char *)m_Buffer1;
			m_WaveHeader1.dwBufferLength = DEV_SAMPLES_PER_BUFFER * format.nChannels * sizeof(short);
			m_WaveHeader1.dwFlags = 0;
			m_WaveHeader1.dwUser = 0;
			waveOutPrepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
			waveOutWrite(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));

			m_WaveHeader2.lpData = (char *)m_Buffer2;
			m_WaveHeader2.dwBufferLength = DEV_SAMPLES_PER_BUFFER * format.nChannels * sizeof(short);
			m_WaveHeader2.dwFlags = 0;
			m_WaveHeader2.dwUser = 0;
			waveOutPrepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
			waveOutWrite(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));

			m_p_packet = std::unique_ptr<AVPacket>(new AVPacket);
			m_packet_ref = false;
		}


		~AudioPlayback()
		{
			waveOutReset(m_WaveOut);
			waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader1, sizeof(WAVEHDR));
			waveOutUnprepareHeader(m_WaveOut, &m_WaveHeader2, sizeof(WAVEHDR));
			waveOutClose(m_WaveOut);
			delete[] m_Buffer;

			if (m_packet_ref)
			{
				av_packet_unref(m_p_packet.get());
				m_packet_ref = false;
			}
		}

	private:
		Player* m_player;
		HWAVEOUT m_WaveOut;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		WAVEHDR	m_WaveHeader1, m_WaveHeader2;

		bool m_packet_ref = false;
		std::unique_ptr<AVPacket> m_p_packet;
		int m_in_pos = 0;
		int m_in_length = 0;
		int m_in_buf_size = 0;

		int m_sync_count = 0;

		static void SoundOutCallBack(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
		{
			if (uMsg == WOM_DONE)
			{
				AudioPlayback* self = (AudioPlayback*)dwInstance;
				Player* player = (Player*)self->m_player;
				PacketQueue& queue = *player->m_queue_audio;
				int time_base_num = player->m_audio_time_base_num;
				int time_base_den = player->m_audio_time_base_den;

				WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
				bool eof = pwhr->dwUser != 0;
				if (!player->m_audio_playing || eof)
				{
					player->m_audio_eof = true;
					return;
				}
				int out_pos = 0;
				int count_packet = 0;
				int64_t progress = -1;
				while (player->m_audio_playing && !eof && out_pos < DEV_SAMPLES_PER_BUFFER)
				{
					if (player->m_audio_buffer != nullptr && self->m_in_pos < self->m_in_length)
					{
						int copy_size = self->m_in_length - self->m_in_pos;
						int copy_size_out = DEV_SAMPLES_PER_BUFFER - out_pos;
						if (copy_size > copy_size_out) copy_size = copy_size_out;
						short* p_out = (short*)pwhr->lpData + out_pos * 2;
						const short* p_in = (const short*)player->m_audio_buffer->data() + self->m_in_pos * 2;
						memcpy(p_out, p_in, sizeof(short)*copy_size * 2);
						out_pos += copy_size;
						self->m_in_pos += copy_size;
					}
					else
					{
						while (true)
						{
							int ret = avcodec_receive_frame(player->m_p_codec_ctx_audio, player->m_p_frm_raw_audio);
							if (ret == 0)
							{
								self->m_in_length = player->m_p_frm_raw_audio->nb_samples;
								if (player->m_audio_buffer == nullptr || self->m_in_length > self->m_in_buf_size)
								{
									self->m_in_buf_size = self->m_in_length;
									player->m_audio_buffer = (std::unique_ptr<AudioBuffer>)(new AudioBuffer(2, self->m_in_buf_size));
									av_samples_fill_arrays(player->m_p_frm_s16_audio->data, player->m_p_frm_s16_audio->linesize,
										player->m_audio_buffer->data(), 2, self->m_in_buf_size, AV_SAMPLE_FMT_S16, 0);
								}
								swr_convert(player->m_swr_ctx, player->m_p_frm_s16_audio->data, self->m_in_length,
									(const uint8_t **)player->m_p_frm_raw_audio->data, self->m_in_length);

								self->m_in_pos = 0;
								break;
							}

							if (self->m_packet_ref)
							{
								av_packet_unref(self->m_p_packet.get());
								self->m_packet_ref = false;
							}

							while (player->m_audio_playing && !eof && queue.Size() == 0)
							{
								if (!player->m_demuxing) eof = true;
							}
							if (!player->m_audio_playing || eof) break;

							*self->m_p_packet = queue.Pop();
							self->m_packet_ref = true;

							if (count_packet == 0)
							{
								while (true)
								{
									progress = self->m_p_packet->dts * time_base_num * AV_TIME_BASE / time_base_den;
									if (progress >= player->m_sync_progress) break;

									avcodec_send_packet(player->m_p_codec_ctx_audio, self->m_p_packet.get());
									while (avcodec_receive_frame(player->m_p_codec_ctx_audio, player->m_p_frm_raw_audio) == 0);
									av_packet_unref(self->m_p_packet.get());
									self->m_packet_ref = false;

									while (player->m_audio_playing && !eof && queue.Size() == 0)
									{
										if (!player->m_demuxing) eof = true;
									}
									if (!player->m_audio_playing || eof) break;
									*self->m_p_packet = queue.Pop();
									self->m_packet_ref = true;
								}
								if (!player->m_audio_playing || eof) break;
							}

							avcodec_send_packet(player->m_p_codec_ctx_audio, self->m_p_packet.get());
							count_packet++;
						}

						if (!player->m_audio_playing || eof) break;
					}
				}

				if (out_pos < DEV_SAMPLES_PER_BUFFER)
				{
					short* p_out = (short*)pwhr->lpData + out_pos * 2;
					size_t bytes = sizeof(short) * (DEV_SAMPLES_PER_BUFFER - out_pos) * 2;
					memset(p_out, 0, bytes);
				}

				static int s_sync_interval = 10;
				if (progress > 0)
				{
					self->m_sync_count = (self->m_sync_count + 1) % s_sync_interval;
					if (self->m_sync_count == 0)
					{
						uint64_t localtime = time_micro_sec();
						player->_set_sync_point(localtime, progress);
					}
				}

				pwhr->dwUser = eof ? 1 : 0;
				waveOutWrite(hwo, pwhr, sizeof(WAVEHDR));
			}
		}
	};

	class Player::VideoPlayback
	{
	public:
		VideoPlayback(Player* player) : m_player(player)
		{
			m_thread_read = (std::unique_ptr<std::thread>)(new std::thread(thread_read, this));
		}


		~VideoPlayback()
		{
			m_thread_read->join();
		}

	private:
		Player* m_player;
		std::unique_ptr<std::thread> m_thread_read;

		static void thread_read(VideoPlayback* self)
		{
			Player* player = (Player*)self->m_player;
			PacketQueue& queue = *player->m_queue_video;

			int time_base_num = player->m_video_time_base_num;
			int time_base_den = player->m_video_time_base_den;

			while (player->m_video_playing && !player->m_video_eof)
			{
				uint64_t localtime, progress;
				player->_get_sync_point(localtime, progress);
				uint64_t t = time_micro_sec();
				int64_t cur_progress = progress + (t - localtime);

				while (player->m_video_playing && !player->m_video_eof && queue.Size() == 0)
				{
					if (!player->m_demuxing)
						player->m_video_eof = true;
				}
				if (!player->m_video_playing || player->m_video_eof) break;

				AVPacket packet = queue.Front();
				int64_t t_next_frame = packet.dts * time_base_num * AV_TIME_BASE / time_base_den;
				if (t_next_frame > cur_progress)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(t_next_frame - cur_progress));
				}
				else
				{
					while (true)
					{
						packet = queue.Pop();
						avcodec_send_packet(player->m_p_codec_ctx_video, &packet);
						avcodec_receive_frame(player->m_p_codec_ctx_video, player->m_p_frm_raw_video);
						av_packet_unref(&packet);

						if (queue.Size() == 0) break;
						AVPacket next_packet = queue.Front();
						t_next_frame = next_packet.dts * time_base_num * AV_TIME_BASE / time_base_den;
						if (t_next_frame > cur_progress) break;
					}

					sws_scale(player->m_sws_ctx, (const uint8_t *const *)player->m_p_frm_raw_video->data, player->m_p_frm_raw_video->linesize,
						0, player->m_p_codec_ctx_video->height, player->m_p_frm_bgr_video->data, player->m_p_frm_bgr_video->linesize);

					for (size_t i = 0; i < player->m_targets.size(); i++)
					{
						player->m_targets[i]->write_image(player->m_video_buffer.get());
					}
				
				}
			}
			player->m_video_eof = true;
		}
	};


	Player::Player(const char* fn, bool play_audio, bool play_video, int audio_device_id) : m_audio_device_id(audio_device_id)
	{
		if (!exists_test(fn))
			printf("Failed loading %s\n", fn);

		avformat_open_input(&m_p_fmt_ctx, fn, nullptr, nullptr);
		avformat_find_stream_info(m_p_fmt_ctx, nullptr);

		m_duration = m_p_fmt_ctx->duration;

		for (unsigned i = 0; i < m_p_fmt_ctx->nb_streams; i++)
		{
			if (play_audio && m_p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				m_a_idx = i;
				m_audio_time_base_num = m_p_fmt_ctx->streams[i]->time_base.num;
				m_audio_time_base_den = m_p_fmt_ctx->streams[i]->time_base.den;
			}
			if (play_video && m_p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				m_v_idx = i;
				m_video_time_base_num = m_p_fmt_ctx->streams[i]->time_base.num;
				m_video_time_base_den = m_p_fmt_ctx->streams[i]->time_base.den;
			}
		}

		// audio
		if (m_a_idx >= 0)
		{
			AVCodecParameters* p_codec_par = m_p_fmt_ctx->streams[m_a_idx]->codecpar;
			AVCodec* p_codec = avcodec_find_decoder(p_codec_par->codec_id);
			m_p_codec_ctx_audio = avcodec_alloc_context3(p_codec);
			avcodec_parameters_to_context(m_p_codec_ctx_audio, p_codec_par);
			avcodec_open2(m_p_codec_ctx_audio, p_codec, nullptr);

			m_p_frm_raw_audio = av_frame_alloc();
			m_p_frm_s16_audio = av_frame_alloc();

			int64_t layout_in = av_get_default_channel_layout(m_p_codec_ctx_audio->channels);
			m_swr_ctx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, m_p_codec_ctx_audio->sample_rate,
				layout_in, m_p_codec_ctx_audio->sample_fmt, m_p_codec_ctx_audio->sample_rate, 0, nullptr);
			swr_init(m_swr_ctx);
		}

		// video
		if (m_v_idx >= 0)
		{
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
		}

		if (m_a_idx >= 0)
		{
			m_queue_audio = (std::unique_ptr<PacketQueue>)(new PacketQueue(42));
		}

		if (m_v_idx >= 0)
		{
			m_queue_video = (std::unique_ptr<PacketQueue>)(new PacketQueue(30));
		}

		m_p_packet = std::unique_ptr<AVPacket>(new AVPacket);

		InitializeCriticalSectionAndSpinCount(&m_cs_sync, 0x00000400);

		m_sync_progress = 0;
	}

	Player::~Player()
	{
		stop();
		DeleteCriticalSection(&m_cs_sync);

		if (m_v_idx >= 0)
		{
			sws_freeContext(m_sws_ctx);
			av_frame_free(&m_p_frm_bgr_video);
			av_frame_free(&m_p_frm_raw_video);
			avcodec_free_context(&m_p_codec_ctx_video);
		}

		if (m_a_idx >= 0)
		{
			swr_free(&m_swr_ctx);
			av_frame_free(&m_p_frm_s16_audio);
			av_frame_free(&m_p_frm_raw_audio);
			avcodec_free_context(&m_p_codec_ctx_audio);
		}
		avformat_close_input(&m_p_fmt_ctx);
	}


	bool Player::is_playing() const
	{
		return m_thread_demux != nullptr;
	}

	bool Player::is_eof_reached() const
	{
		return m_thread_demux != nullptr && m_audio_eof && m_video_eof;
	}


	uint64_t Player::get_position() const
	{
		if (m_thread_demux == nullptr)
		{
			return m_sync_progress;
		}
		else if (m_audio_eof && m_video_eof)
		{
			return get_duration();
		}
		else
		{
			uint64_t localtime, progress;
			_get_sync_point(localtime, progress);
			return progress + (time_micro_sec() - localtime);
		}
	}


	void Player::stop()
	{
		if (m_thread_demux != nullptr)
		{
			uint64_t pos = get_position();

			m_demuxing = false;
			m_audio_playing = false;
			m_video_playing = false;
			m_audio_playback = nullptr;
			m_video_playback = nullptr;

			while (m_a_idx >= 0 && m_queue_audio->Size() > 0)
			{
				AVPacket packet = m_queue_audio->Pop();
				av_packet_unref(&packet);
			}

			while (m_v_idx >= 0 && m_queue_video->Size() > 0)
			{
				AVPacket packet = m_queue_video->Pop();
				av_packet_unref(&packet);
			}

			m_thread_demux->join();
			m_thread_demux = nullptr;

			m_sync_progress = pos;
		}
	}


	void Player::_start(uint64_t pos)
	{
		stop();
		avformat_seek_file(m_p_fmt_ctx, -1, INT64_MIN, pos, INT64_MAX, 0);
		m_sync_local_time = time_micro_sec();
		m_sync_progress = pos;

		m_demuxing = true;
		m_thread_demux = (std::unique_ptr<std::thread>)(new std::thread(thread_demux, this));

		if (m_a_idx >= 0)
		{
			m_audio_playing = true;
			m_audio_eof = false;
			m_audio_playback = (std::unique_ptr<AudioPlayback>)(new AudioPlayback(m_audio_device_id, this));
		}

		if (m_v_idx >= 0)
		{
			m_video_playing = true;
			m_video_eof = false;
			m_video_playback = (std::unique_ptr<VideoPlayback>)(new VideoPlayback(this));
		}
	}


	void Player::start()
	{
		if (m_thread_demux == nullptr)
		{
			_start(m_sync_progress);
		}
	}


	void Player::set_position(uint64_t pos)
	{
		if (m_thread_demux != nullptr)
		{
			_start(pos);
		}
		else
		{
			if (m_v_idx >= 0)
			{
				avformat_seek_file(m_p_fmt_ctx, -1, INT64_MIN, pos, INT64_MAX, 0);

				bool frame_read = false;
				while (av_read_frame(m_p_fmt_ctx, m_p_packet.get()) == 0)
				{
					if (m_p_packet->stream_index == m_v_idx)
					{
						int64_t t_frame = m_p_packet->dts * m_video_time_base_num * AV_TIME_BASE / m_video_time_base_den;
						avcodec_send_packet(m_p_codec_ctx_video, m_p_packet.get());
						avcodec_receive_frame(m_p_codec_ctx_video, m_p_frm_raw_video);
						av_packet_unref(m_p_packet.get());
						if (t_frame >= pos)
						{
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
				
				if (frame_read)
				{
					for (size_t i = 0; i < m_targets.size(); i++)
					{
						m_targets[i]->write_image(m_video_buffer.get());
					}
				}
			}
			m_sync_progress = pos;
		}
	}


	void Player::set_audio_device(int audio_device_id)
	{
		if (audio_device_id != m_audio_device_id)
		{
			m_audio_device_id = audio_device_id;
			if (m_a_idx >= 0 && m_thread_demux != nullptr)
			{
				m_audio_playing = false;
				m_audio_playback = nullptr;
				m_audio_playing = true;
				m_audio_eof = false;
				m_audio_playback = (std::unique_ptr<AudioPlayback>)(new AudioPlayback(m_audio_device_id, this));
			}
		}
	}


	void Player::_set_sync_point(uint64_t local_time, uint64_t progress)
	{
		EnterCriticalSection(&m_cs_sync);
		m_sync_local_time = local_time;
		m_sync_progress = progress;
		LeaveCriticalSection(&m_cs_sync);
	}

	void Player::_get_sync_point(uint64_t& local_time, uint64_t& progress) const
	{
		EnterCriticalSection(&m_cs_sync);
		local_time = m_sync_local_time;
		progress = m_sync_progress;
		LeaveCriticalSection(&m_cs_sync);
	}


	void Player::thread_demux(Player* self)
	{
		while (self->m_demuxing && av_read_frame(self->m_p_fmt_ctx, self->m_p_packet.get()) == 0)
		{
			if (self->m_p_packet->stream_index == self->m_a_idx)
			{
				self->m_queue_audio->Push(*self->m_p_packet);
			}
			else if (self->m_p_packet->stream_index == self->m_v_idx)
			{
				self->m_queue_video->Push(*self->m_p_packet);
			}
			else
			{
				av_packet_unref(self->m_p_packet.get());
			}
		}
		self->m_demuxing = false;
	}

}