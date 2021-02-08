#include "Camera.h"
#include "Image.h"
#include "VideoPort.h"
#include "Utils.h"


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <dshow.h>
#include <thread>

namespace LiveKit
{

	inline std::string BSTRtoANSI(BSTR bstr)
	{
		int len = (int)SysStringLen(bstr);
		if (len == 0) return "";
		int size_needed = WideCharToMultiByte(CP_ACP, 0, bstr, len, nullptr, 0, 0, 0);
		std::vector<char> ret(size_needed + 1, (char)0);
		WideCharToMultiByte(CP_ACP, 0, bstr, len, ret.data(), (int)ret.size(), 0, 0);
		return ret.data();
	}

	inline void EnumerateCameras(std::vector<std::string>& lst_names)
	{
		lst_names.clear();
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		IEnumMoniker *pEnum;
		ICreateDevEnum *pDevEnum;
		CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
		pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
		{
			IMoniker *pMoniker = nullptr;
			while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
			{
				IPropertyBag *pPropBag;
				pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				VARIANT var;
				VariantInit(&var);
				pPropBag->Read(L"FriendlyName", &var, 0);
				lst_names.push_back(BSTRtoANSI(var.bstrVal));
				VariantClear(&var);
				pPropBag->Release();
				pMoniker->Release();
			}
		}
		pEnum->Release();
		CoUninitialize();
	}


	const std::vector<std::string>* Camera::s_get_list_devices()
	{
		static std::vector<std::string> s_list_devices;
		if (s_list_devices.size() == 0)
			EnumerateCameras(s_list_devices);
		return &s_list_devices;
	}


	Camera::Camera(int idx)
	{
		static bool s_first_time = true;
		if (s_first_time)
		{
			avdevice_register_all();
			s_first_time = false;
		}

		const auto* lst = s_get_list_devices();
		std::string url = std::string("video=") + (*lst)[idx];

		AVInputFormat *inFrmt = av_find_input_format("dshow");
		m_p_fmt_ctx = nullptr;
		avformat_open_input(&m_p_fmt_ctx, url.c_str(), inFrmt, nullptr);
		avformat_find_stream_info(m_p_fmt_ctx, nullptr);

		m_v_idx = -1;
		for (unsigned i = 0; i < m_p_fmt_ctx->nb_streams; i++)
		{
			if (m_p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				m_v_idx = i;
				m_frame_rate_num = m_p_fmt_ctx->streams[i]->avg_frame_rate.num;
				m_frame_rate_den = m_p_fmt_ctx->streams[i]->avg_frame_rate.den;
				break;
			}
		}

		AVCodecParameters* p_codec_par = m_p_fmt_ctx->streams[m_v_idx]->codecpar;
		AVCodec* p_codec = avcodec_find_decoder(p_codec_par->codec_id);
		m_p_codec_ctx = avcodec_alloc_context3(p_codec);
		avcodec_parameters_to_context(m_p_codec_ctx, p_codec_par);
		avcodec_open2(m_p_codec_ctx, p_codec, nullptr);

		m_width = m_p_codec_ctx->width;
		m_height = m_p_codec_ctx->height;
		m_img = (std::unique_ptr<Image>)(new Image(m_width, m_height));	

		m_p_frm_raw = av_frame_alloc();
		m_p_frm_bgr = av_frame_alloc();

		av_image_fill_arrays(m_p_frm_bgr->data, m_p_frm_bgr->linesize, m_img->data(), AV_PIX_FMT_BGR24, m_width, m_height, 1);
		m_sws_ctx = sws_getContext(m_width, m_height, m_p_codec_ctx->pix_fmt, m_width, m_height, AV_PIX_FMT_BGR24, SWS_BICUBIC, nullptr, nullptr, nullptr);

		m_p_packet = std::unique_ptr<AVPacket>(new AVPacket);
		m_thread_read = (std::unique_ptr<std::thread>)(new std::thread(thread_read, this));
		m_start_time = time_micro_sec();
	}


	Camera::~Camera()
	{
		m_quit = true;
		m_thread_read->join();

		sws_freeContext(m_sws_ctx);
		av_frame_free(&m_p_frm_bgr);
		av_frame_free(&m_p_frm_raw);
		avcodec_free_context(&m_p_codec_ctx);
		avformat_close_input(&m_p_fmt_ctx);
	}


	void Camera::thread_read(Camera* self)
	{
		while (!self->m_quit)
		{
			self->m_frame_count++;
			uint64_t target = self->m_start_time + self->m_frame_count * self->m_frame_rate_den * AV_TIME_BASE / self->m_frame_rate_num;
			uint64_t now = time_micro_sec();
			int64_t delta = target - now;
			if (delta > 0)
				std::this_thread::sleep_for(std::chrono::microseconds(delta));

			while (true)
			{
				av_read_frame(self->m_p_fmt_ctx, self->m_p_packet.get());
				if (self->m_p_packet->stream_index == self->m_v_idx) break;
				else
				{
					av_packet_unref(self->m_p_packet.get());
				}
			}
			avcodec_send_packet(self->m_p_codec_ctx, self->m_p_packet.get());
			avcodec_receive_frame(self->m_p_codec_ctx, self->m_p_frm_raw);
			av_packet_unref(self->m_p_packet.get());

			sws_scale(self->m_sws_ctx, (const uint8_t *const *)self->m_p_frm_raw->data, self->m_p_frm_raw->linesize, 0, self->m_p_codec_ctx->height, self->m_p_frm_bgr->data, self->m_p_frm_bgr->linesize);

			for (size_t i = 0; i < self->m_targets.size(); i++)
			{
				self->m_targets[i]->write_image(self->m_img.get());
			}
		}
	}




}