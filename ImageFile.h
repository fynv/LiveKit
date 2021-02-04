#pragma once

#include "VideoPort.h"
#include <memory>

namespace LiveKit
{

	class Image;
	class ImageFile : public VideoSource
	{
	public:
		ImageFile(const char* filename);
		~ImageFile();

		int width() const;
		int height() const;

		virtual const Image* read_image(uint64_t* timestamp) const;

	private:
		uint64_t m_timestamp;
		std::unique_ptr<Image> m_image;
	};

}