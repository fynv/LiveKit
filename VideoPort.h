#pragma once
#include <cstdint>
#include <memory>

namespace LiveKit
{
	class Image;

	class VideoSource
	{
	public:
		VideoSource() {}
		virtual ~VideoSource() {}
		virtual const Image* read_image(uint64_t* timestamp) const = 0;
	};

	class VideoTarget
	{
	public:
		VideoTarget() {}
		virtual ~VideoTarget() {}
		virtual void write_image(const Image* image) = 0;
	};

	class VideoPort : public VideoTarget, public VideoSource
	{
	public:
		VideoPort();
		~VideoPort();

		virtual void write_image(const Image* image);
		virtual const Image* read_image(uint64_t* timestamp) const;		

	private:
		std::unique_ptr<Image> m_video_bufs[3];
		uint64_t m_timestamps[3];
		int m_last_video_buf = 2;
	};

}
