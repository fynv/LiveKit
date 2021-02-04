#pragma once

#include "VideoPort.h"
#include <memory>
#include <vector>
#include <string>

namespace LiveKit
{
	class Image;
	class WindowCapture : public VideoSource
	{
	public:
		WindowCapture(void *hwnd);
		WindowCapture(int idx);
		~WindowCapture();

		virtual const Image* read_image(uint64_t* timestamp) const;
		static const std::vector<std::string>* s_get_window_titles();

	private:
		static std::vector<std::string> s_win_titles;
		static std::vector<void*> s_win_handles;
		static void s_update_win_info();

		void* m_hwnd;
		mutable std::unique_ptr<Image> m_image;
	};

}
