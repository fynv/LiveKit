#include <Windows.h>
#include "Registry.h"

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
		Unmap();
		if (m_hMapFile != NULL)
		{
			CloseHandle(m_hMapFile);
		}
	}

	unsigned char* Fetch(bool& is_flipped)
	{
		Unmap();
		int width = VIDEO_WIDTH;
		int height = VIDEO_HEIGHT;
		int chn = 3;
		size_t frame_size = sizeof(FrameHeader) + width * height*chn;
		size_t total_size = sizeof(Header) + frame_size * 3;

		is_flipped = false;
		if (m_hMapFile == NULL)
		{
			m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME.c_str());
			if (m_hMapFile == NULL) return nullptr;
			m_data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
			if (m_data == nullptr) return nullptr;
		}
		else
		{
			m_data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
			if (m_data == nullptr)
			{
				CloseHandle(m_hMapFile);
				m_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAME.c_str());
				if (m_hMapFile == NULL) return nullptr;
				m_data = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, total_size);
				if (m_data == nullptr) return nullptr;
			}
		}

		Header* header = (Header*)m_data;
		int cur_frame = header->cur_frame;
		unsigned char* p_frames = (unsigned char*)(header+1);
		FrameHeader* frame_header = (FrameHeader*)(p_frames + frame_size * cur_frame);
		is_flipped = frame_header->is_flipped != 0;
		return (unsigned char*)(frame_header + 1);	
	}

	void Unmap()
	{
		if (m_data != nullptr)
		{
			UnmapViewOfFile(m_data);
			m_data = nullptr;
		}
	}

private:
	HANDLE m_hMapFile = NULL;
	void* m_data = nullptr;
};

unsigned char* FetchImage(bool& is_flipped)
{
	static Fetcher fetcher;
	return fetcher.Fetch(is_flipped);
}
