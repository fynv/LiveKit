#pragma once

#include "VideoPort.h"
#include <Windows.h>

namespace LiveKit
{
	class IPCTarget : public VideoTarget
	{
	public:
		IPCTarget(const char* mapping_name, int width, int height, bool has_alpha = false);
		~IPCTarget();

		virtual void write_image(const Image* image);

	private:
		HANDLE m_hMapFile = nullptr;
		void* m_data = nullptr;
		struct Header;
		struct FrameHeader;

	};


}
