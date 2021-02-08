#pragma once

#include <memory>
#include <vector>

struct GLFWwindow;

namespace LiveKit
{
	class Image;
	class VideoSource;
	class VideoTarget;
	class Compositor
	{
	public:
		Compositor(int video_width, int video_height, int window_width, int window_height, const char* title);
		~Compositor();

		void SetVideoResolution(int video_width, int video_height)
		{
			m_width_video = video_width;
			m_height_video = video_height;
		}

		int VideoWidth() const { return m_width_video; }
		int VideoHeight() const { return m_height_video; }

		void SetSource(int i, const VideoSource* source);
		void SetSource(int i, const VideoSource* source, int pos_x, int pos_y);
		void SetSource(int i, const VideoSource* source, int pos_x, int pos_y, int pos_x2, int pos_y2);
		void RemoveSource(int i);

		void SetMargin(int margin) { m_margin = margin; }
		bool Draw();

		void AddTarget(VideoTarget* target)
		{
			m_targets.push_back(target);
		}

	private:
		GLFWwindow* m_window = nullptr;
		int m_width_video, m_height_video;
		int m_width_video_last = -1;
		int m_height_video_last = -1;
		int m_margin = 0;

		unsigned m_tex_video = -1;
		unsigned m_fbo_video = -1;

		struct Layer;

		std::vector<std::unique_ptr<Layer>> m_layers;

		std::unique_ptr<Image> m_img;
		std::vector<VideoTarget*> m_targets;

	};


}
