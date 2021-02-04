#include "VideoPort.h"
#include "Image.h"
#include "Utils.h"

namespace LiveKit
{
	VideoPort::VideoPort()
	{
		memset(m_timestamps, 0xFF, sizeof(uint64_t) * 3);
	}

	VideoPort::~VideoPort()
	{

	}
	
	void VideoPort::write_image(const Image* image)
	{
		int this_buf = m_last_video_buf % 3;
		m_timestamps[this_buf] = time_micro_sec();		
		std::unique_ptr<Image>& p_this_buf = m_video_bufs[this_buf];
		if (p_this_buf == nullptr || p_this_buf->width() != image->width() || p_this_buf->height() != image->height() || p_this_buf->has_alpha() != image->has_alpha())
		{
			p_this_buf = std::unique_ptr<Image>(new Image(*image));
		}
		else
		{
			*p_this_buf = *image;
		}
		m_last_video_buf = this_buf;
	}
	
	const Image* VideoPort::read_image(uint64_t* timestamp) const
	{
		*timestamp = m_timestamps[m_last_video_buf];
		return m_video_bufs[m_last_video_buf].get();
	}

}