#include "IPCTarget.h"
#include "Image.h"
#include "Utils.h"

namespace LiveKit
{
	struct IPCTarget::Header
	{
		int width;
		int height;
		int chn;
		int cur_frame;
	};

	struct IPCTarget::FrameHeader
	{
		int is_flipped;
		uint64_t timestamp;
	};

	IPCTarget::IPCTarget(const char* mapping_name, int width, int height, bool has_alpha)
	{
		int chn = has_alpha ? 4 : 3;
		size_t total_size = sizeof(Header) + (sizeof(FrameHeader) + width*height*chn) * 3;
		m_hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)(total_size), mapping_name);
		m_data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
		memset(m_data, 0, total_size);
		*(Header*)m_data = { width, height, chn, 2 };
	}

	IPCTarget::~IPCTarget()
	{
		UnmapViewOfFile(m_data);
		CloseHandle(m_hMapFile);
	}

	void IPCTarget::write_image(const Image* image)
	{
		Header* header = (Header*)m_data;
		int width_out = header->width;
		int height_out = header->height;
		int chn_out = header->chn;
		int cur_frame = header->cur_frame;

		size_t frame_size = sizeof(FrameHeader) + width_out * height_out *chn_out;
		unsigned char* p_frames = (unsigned char*)(header+1);
		int write_frame = (cur_frame + 1) % 3;

		FrameHeader* frame_header = (FrameHeader*)(p_frames + frame_size * write_frame);
		frame_header->is_flipped = image->is_flipped()?1:0;
		frame_header->timestamp = time_micro_sec();

		unsigned char* p_data_out = (unsigned char*)(frame_header + 1);
		copy_centered(image->data(), image->width(), image->height(), image->has_alpha() ? 4 : 3,
			p_data_out, width_out, height_out, chn_out, false);

		header->cur_frame = write_frame;

	}

}
