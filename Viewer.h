#pragma once

#include <memory>

struct GLFWwindow;

namespace LiveKit
{	
	class VideoSource;
	class Viewer
	{
	public:
		Viewer(int window_width, int window_height, const char* title);
		~Viewer();

		void SetSource(const VideoSource* source)
		{
			m_source = source;
		}

		bool Draw();

	private:
		GLFWwindow* m_window = nullptr;
		const VideoSource* m_source = nullptr;
		uint64_t m_timestamp = (uint64_t)(-1);
		unsigned m_tex_id;
		bool m_flipped = false;
	};
}