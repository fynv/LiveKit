#include "WindowCapture.h"
#include "Image.h"
#include "Utils.h"
#include <Windows.h>

namespace LiveKit
{
	struct WinInfo
	{
		std::vector<std::string>* titles;
		std::vector<void*>* handles;
	};

	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
	{
		if (IsWindowVisible(hwnd))
		{
			int length = GetWindowTextLength(hwnd);
			if (length > 0)
			{
				std::vector<char> buffer(length + 1, 0);
				GetWindowTextA(hwnd, buffer.data(), length + 1);

				WinInfo* wi = (WinInfo*)lParam;
				wi->titles->push_back(buffer.data());
				wi->handles->push_back(hwnd);
			}
		}

		return TRUE;
	}

	std::vector<std::string> WindowCapture::s_win_titles;
	std::vector<void*> WindowCapture::s_win_handles;

	void WindowCapture::s_update_win_info()
	{
		s_win_titles.clear();
		s_win_handles.clear();
		WinInfo info = { &s_win_titles, &s_win_handles };
		EnumWindows(EnumWindowsProc, (LPARAM)(&info));
	}
	
	const std::vector<std::string>* WindowCapture::s_get_window_titles()
	{
		s_update_win_info();
		return &s_win_titles;
	}

	WindowCapture::WindowCapture(void *hwnd) : m_hwnd(hwnd)
	{

	}

	WindowCapture::WindowCapture(int idx)
	{
		if (s_win_handles.size() == 0) s_update_win_info();
		m_hwnd = s_win_handles[idx];
	}

	WindowCapture::~WindowCapture()
	{

	}

	const Image* WindowCapture::read_image(uint64_t* timestamp) const
	{
		HDC hdcWindow = GetDC((HWND)m_hwnd);
		HDC hdcMemDC = CreateCompatibleDC(hdcWindow);

		RECT rcClient;
		GetClientRect((HWND)m_hwnd, &rcClient);
		int width = rcClient.right - rcClient.left;
		int height = rcClient.bottom - rcClient.top;
		
		HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, width, height);
		SelectObject(hdcMemDC, hbm);
		BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

		if (m_image == nullptr || m_image->width() != width || m_image->height() != height)
		{
			m_image = (std::unique_ptr<Image>)(new Image(width, height, true));
		}

		BITMAPINFOHEADER bi;
		memset(&bi, 0, sizeof(BITMAPINFOHEADER));
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		bi.biHeight = -height;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = BI_RGB;

		GetDIBits(hdcWindow, hbm, 0, (UINT)height, m_image->data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

		DeleteObject(hbm);
		DeleteObject(hdcMemDC);
		ReleaseDC((HWND)m_hwnd, hdcWindow);

		*timestamp = time_micro_sec();

		return m_image.get();
	}

}
