#pragma once

#include "VideoPort.h"
#include <Windows.h>
#include <string>

namespace LiveKit
{
	class IPCSource : public VideoSource
	{
	public:
		IPCSource(const char* mapping_name);
		~IPCSource();

		virtual const Image* read_image(uint64_t* timestamp) const;

	private:
		std::string m_mapping_name;
		mutable HANDLE m_hMapFile = nullptr;
		mutable std::unique_ptr<Image> m_frame;
		mutable uint64_t m_timestamp = (uint64_t)(-1);
		struct Header;
		struct FrameHeader;
		

	};

}

