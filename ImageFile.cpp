#include "ImageFile.h"
#include "Image.h"
#include "Utils.h"

namespace LiveKit
{
	ImageFile::ImageFile(const char* filename)
		:m_image(new Image(filename, true))
	{
		m_timestamp = time_micro_sec();
		
	}

	ImageFile::~ImageFile()
	{

	}

	int ImageFile::width() const
	{
		return m_image->width();
	}

	int ImageFile::height() const
	{
		return m_image->height();
	}
	
	const Image* ImageFile::read_image(uint64_t* timestamp) const
	{
		*timestamp = m_timestamp;
		return m_image.get();
	}

}