#include <Windows.h>
#include "Registry.h"


inline void copy_centered_samechn(const uint8_t* data_in, int width_in, int height_in,
	uint8_t* data_out, int width_out, int height_out, int chn, bool flip)
{
	int offset_in_x = 0;
	int offset_in_y = 0;
	int offset_out_x = 0;
	int offset_out_y = 0;
	int scan_w = min(width_out, width_in);
	int scan_h = min(height_out, height_in);

	if (width_in < width_out)
		offset_out_x = (width_out - width_in) / 2;
	else if (width_in > width_out)
		offset_in_x = (width_in - width_out) / 2;

	if (height_in < height_out)
		offset_out_y = (height_out - height_in) / 2;
	else if (height_in > height_out)
		offset_in_y = (height_in - height_out) / 2;

	for (int y = 0; y < scan_h; y++)
	{
		const uint8_t* p_in_line;
		if (!flip)
		{
			p_in_line = data_in + (y + offset_in_y)*width_in*chn;
		}
		else
		{
			int y2 = (scan_h - 1) - y;
			p_in_line = data_in + (y2 + offset_in_y)*width_in*chn;
		}

		uint8_t* p_out_line = data_out + (y + offset_out_y)*width_out * chn;

		const uint8_t* p_in = p_in_line + offset_in_x * chn;
		uint8_t* p_out = p_out_line + offset_out_x * chn;
		memcpy(p_out, p_in, scan_w*chn);
	}
}

inline void copy_centered(const uint8_t* data_in, int width_in, int height_in, int chn_in,
	uint8_t* data_out, int width_out, int height_out, int chn_out, bool flip)
{
	memset(data_out, 0, width_out*height_out*chn_out);

	if (chn_in == chn_out)
		return copy_centered_samechn(data_in, width_in, height_in, data_out, width_out, height_out, chn_in, flip);

	int offset_in_x = 0;
	int offset_in_y = 0;
	int offset_out_x = 0;
	int offset_out_y = 0;
	int scan_w = min(width_out, width_in);
	int scan_h = min(height_out, height_in);

	if (width_in < width_out)
		offset_out_x = (width_out - width_in) / 2;
	else if (width_in > width_out)
		offset_in_x = (width_in - width_out) / 2;

	if (height_in < height_out)
		offset_out_y = (height_out - height_in) / 2;
	else if (height_in > height_out)
		offset_in_y = (height_in - height_out) / 2;

	for (int y = 0; y < scan_h; y++)
	{
		const uint8_t* p_in_line;
		if (!flip)
		{
			p_in_line = data_in + (y + offset_in_y)*width_in*chn_in;
		}
		else
		{
			int y2 = (scan_h - 1) - y;
			p_in_line = data_in + (y2 + offset_in_y)*width_in*chn_in;
		}

		uint8_t* p_out_line = data_out + (y + offset_out_y)*width_out * chn_out;
		for (int x = 0; x < scan_w; x++)
		{
			const uint8_t* p_in = p_in_line + (x + offset_in_x)*chn_in;
			uint8_t* p_out = p_out_line + (x + offset_out_x) * chn_out;
			int c = 0;
			for (; c < min(chn_in, chn_out); c++)
				p_out[c] = p_in[c];
			for (; c < chn_out; c++)
				p_out[c] = 255;
		}
	}
}

struct Header
{
	int width;
	int height;
	int chn;
	int cur_frame;
};

struct FrameHeader
{
	int is_flipped;
	uint64_t timestamp;
};

class Fetcher
{
public:
	Fetcher() {}
	~Fetcher()
	{
		if (m_hMapFile != nullptr)
		{
			CloseHandle(m_hMapFile);
		}
	}

	bool Fetch(unsigned char* dst)
	{
		void* data = nullptr;
		int width_out = VIDEO_WIDTH;
		int height_out = VIDEO_HEIGHT;
		int chn_out = 3;		

		if (m_hMapFile == nullptr)
		{
			m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME.c_str());
			if (m_hMapFile == nullptr) return false;
			data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
			if (data == nullptr) return false;
		}
		else
		{
			data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
			if (data == nullptr)
			{
				CloseHandle(m_hMapFile);
				m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME.c_str());
				if (m_hMapFile == nullptr) return false;
				data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Header));
				if (data == nullptr) return false;
			}
		}

		Header* header = (Header*)data;
		int width_in = header->width;
		int height_in = header->height;
		int chn_in = header->chn;
		int cur_frame = header->cur_frame;
		size_t frame_size = sizeof(FrameHeader) + width_in * height_in * chn_in;
		size_t total_size = sizeof(Header) + frame_size * 3;

		UnmapViewOfFile(data);
		data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
		
		unsigned char* p_frames = (unsigned char*)data + sizeof(Header);
		FrameHeader* frame_header = (FrameHeader*)(p_frames + frame_size * cur_frame);
		uint64_t new_timestamp = frame_header->timestamp;

		bool copied = false;
		if (new_timestamp != m_timestamp)
		{
			bool is_flipped = frame_header->is_flipped != 0;
			unsigned char* data_in = (unsigned char*)(frame_header + 1);
			copy_centered(data_in, width_in, height_in, chn_in, dst, width_out, height_out, chn_out, !is_flipped);
			m_timestamp = new_timestamp;
			copied = true;
		}
		UnmapViewOfFile(data);		
		return copied;
	}

private:
	HANDLE m_hMapFile = nullptr;
	uint64_t m_timestamp = (uint64_t)(-1);
};

bool FetchImage(unsigned char* dst)
{
	static Fetcher fetcher;
	return fetcher.Fetch(dst);
}
