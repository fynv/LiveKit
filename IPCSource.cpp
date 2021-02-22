#include "IPCSource.h"
#include "Image.h"
#include "Utils.h"

namespace LiveKit
{
	struct IPCSource::Header
	{
		int width;
		int height;
		int chn;
		int cur_frame;
	};

	struct IPCSource::FrameHeader
	{
		int is_flipped;
		uint64_t timestamp;
	};

	IPCSource::IPCSource(const char* mapping_name)
	{
		m_mapping_name = mapping_name;
	}

	IPCSource::~IPCSource()
	{
		if (m_hMapFile != nullptr)
		{
			CloseHandle(m_hMapFile);
		}
	}

	const Image* IPCSource::read_image(uint64_t* timestamp) const
	{
		*timestamp = m_timestamp;

		void *data = nullptr;
		if (m_hMapFile == nullptr)
		{
			m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_mapping_name.c_str());
			if (m_hMapFile == nullptr) return m_frame.get();
			data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
			if (data == nullptr) return m_frame.get();
		}
		else
		{
			data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
			if (data == nullptr)
			{
				CloseHandle(m_hMapFile);
				m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_mapping_name.c_str());
				if (m_hMapFile == nullptr) return m_frame.get();
				data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
				if (data == nullptr) return m_frame.get();
			}
		}

		Header* header = (Header*)data;
		int width = header->width;
		int height = header->height;
		int chn = header->chn;
		int cur_frame = header->cur_frame;
		size_t frame_size = sizeof(FrameHeader) + width * height*chn;
		size_t total_size = sizeof(Header) + frame_size * 3;

		UnmapViewOfFile(data);
		data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
		unsigned char* p_frames = (unsigned char*)data + sizeof(Header);
		FrameHeader* frame_header = (FrameHeader*)(p_frames + frame_size * cur_frame);
		uint64_t ts = frame_header->timestamp;
		if (ts != m_timestamp)
		{			
			if (m_frame == nullptr || m_frame->width() != width || m_frame->height() != height || m_frame->has_alpha() != (chn>3))			
				m_frame = (std::unique_ptr<Image>)(new Image(width, height, chn > 3));
			bool is_flipped = frame_header->is_flipped!=0;
			m_frame->set_flipped(is_flipped);
			const unsigned char* p_data = (const unsigned char*)(frame_header + 1);
			memcpy(m_frame->data(), p_data, width * height*chn);
			m_timestamp = ts;
			*timestamp = m_timestamp;
		}

		UnmapViewOfFile(data);

		return m_frame.get();
	}


}