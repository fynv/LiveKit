#include "Compositor.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Image.h"
#include "VideoPort.h"
#include "RenderingOGL.h"

namespace LiveKit
{
	struct Compositor::Layer
	{
		Layer(const VideoSource* src)
		{
			source = src;
			glGenTextures(1, &tex);
		}
		~Layer()
		{
			glDeleteTextures(1, &tex);
		}

		const  VideoSource* source;
		uint64_t timestamp = (uint64_t)(-1);

		unsigned tex;
		int width = -1;
		int height = -1;
		bool alpha_blend = false;
		bool flipped = false;

		enum class Mode
		{
			Stretch,
			Move,
			StretchMove
		};

		Mode mode = Mode::Stretch;
		int pos_x = 0;
		int pos_y = 0;
		int pos_x2 = 0;
		int pos_y2 = 0;

		void SetStretch()
		{
			mode = Mode::Stretch;
			pos_x = 0;
			pos_y = 0;
		}

		void SetPosition(int pos_x, int pos_y)
		{
			mode = Mode::Move;
			this->pos_x = pos_x;
			this->pos_y = pos_y;
		}

		void SetPosition(int pos_x, int pos_y, int pos_x2, int pos_y2)
		{
			mode = Mode::StretchMove;
			this->pos_x = pos_x;
			this->pos_y = pos_y;
			this->pos_x2 = pos_x2;
			this->pos_y2 = pos_y2;
		}

		void Draw(int video_width, int video_height)
		{
			if (source != nullptr)
			{
				uint64_t new_timestamp;
				const Image* image = source->read_image(&new_timestamp);
				if (new_timestamp != (uint64_t)(-1) && new_timestamp != timestamp)
				{
					timestamp = new_timestamp;
					width = image->width();
					height = image->height();
					alpha_blend = image->has_alpha();
					flipped = image->is_flipped();

					glBindTexture(GL_TEXTURE_2D, tex);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					if (image->has_alpha())
					{
						glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image->data());
					}
					else
					{
						glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, image->data());
					}
					glBindTexture(GL_TEXTURE_2D, 0);

				}
			}

			if (alpha_blend)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				glDisable(GL_BLEND);
			}

			if (mode == Mode::Stretch)
			{
				glViewport(0, 0, video_width, video_height);
			}
			else if (mode == Mode::Move)
			{
				glViewport(pos_x, video_height - (pos_y + height), width, height);
			}
			else if (mode == Mode::StretchMove)
			{
				glViewport(pos_x, video_height - pos_y2, pos_x2 - pos_x, pos_y2 - pos_y);
			}

			DrawFullScreen(tex, flipped);

		}
	};


	Compositor::Compositor(int video_width, int video_height, int window_width, int window_height, const char* title)
		: m_width_video(video_width), m_height_video(video_height)
	{
		glfwInit();
		m_window = glfwCreateWindow(window_width, window_height, title, NULL, NULL);
		glfwMakeContextCurrent(m_window);
		glewInit();
	}

	Compositor::~Compositor()
	{
		if (m_fbo_video != -1)
			glDeleteFramebuffers(1, &m_fbo_video);
		if (m_tex_video != -1)
			glDeleteTextures(1, &m_tex_video);

		m_layers.clear();

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void Compositor::SetSource(int i, const VideoSource* source)
	{
		if (i >= (int)m_layers.size()) m_layers.resize(i + 1);
		m_layers[i] = (std::unique_ptr<Layer>)(new Layer(source));
	}

	void Compositor::SetSource(int i, const VideoSource* source, int pos_x, int pos_y)
	{
		if (i >= (int)m_layers.size()) m_layers.resize(i + 1);
		m_layers[i] = (std::unique_ptr<Layer>)(new Layer(source));
		m_layers[i]->SetPosition(pos_x, pos_y);
	}

	void Compositor::SetSource(int i, const VideoSource* source, int pos_x, int pos_y, int pos_x2, int pos_y2)
	{
		if (i >= (int)m_layers.size()) m_layers.resize(i + 1);
		m_layers[i] = (std::unique_ptr<Layer>)(new Layer(source));
		m_layers[i]->SetPosition(pos_x, pos_y, pos_x2, pos_y2);
	}

	void Compositor::RemoveSource(int i)
	{
		if (i < (int)m_layers.size()) m_layers[i] = nullptr;
	}

	bool Compositor::Draw()
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(m_window)) return false;

		GLint backbufId = 0;
		int width_wnd, height_wnd;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &backbufId);
		glfwGetFramebufferSize(m_window, &width_wnd, &height_wnd);

		if (m_width_video_last != m_width_video || m_height_video_last != m_height_video)
		{
			if (m_fbo_video == -1)
			{
				glGenFramebuffers(1, &m_fbo_video);
				glGenTextures(1, &m_tex_video);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_video);

			glBindTexture(GL_TEXTURE_2D, m_tex_video);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, m_width_video, m_height_video, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_video, 0);

			m_width_video_last = m_width_video;
			m_height_video_last = m_height_video;
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_video);
		}

		glEnable(GL_FRAMEBUFFER_SRGB);

		glViewport(0, 0, m_width_video, m_height_video);
		glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for (size_t i = 0; i < m_layers.size(); i++)
			if (m_layers[i] != nullptr)
				m_layers[i]->Draw(m_width_video, m_height_video);

		if (m_targets.size() > 0)
		{
			if (m_img == nullptr || m_img->width() != m_width_video || m_img->height() != m_height_video)
			{
				m_img = (std::unique_ptr<Image>)(new Image(m_width_video, m_height_video, false));
				m_img->set_flipped(true);
			}
			glReadPixels(0, 0, m_width_video, m_height_video, GL_BGR, GL_UNSIGNED_BYTE, m_img->data());

			for (size_t i = 0; i < m_targets.size(); i++)
			{
				m_targets[i]->write_image(m_img.get());
			}
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backbufId);
		glDisable(GL_FRAMEBUFFER_SRGB);

		glViewport(0, 0, width_wnd, height_wnd);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		int client_w = width_wnd - m_margin * 2;
		int client_h = height_wnd - m_margin * 2;

		if (client_w < m_width_video || client_h < m_height_video)
		{
			// scale down
			int dst_w = m_width_video * client_h / m_height_video;
			if (dst_w <= client_w)
			{
				int dst_offset = (client_w - dst_w) / 2;
				glBlitFramebuffer(0, 0, m_width_video, m_height_video, m_margin + dst_offset, m_margin, m_margin + dst_offset + dst_w, m_margin + client_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			}
			else
			{
				int dst_h = m_height_video * client_w / m_width_video;
				int dst_offset = (client_h - dst_h) / 2;
				glBlitFramebuffer(0, 0, m_width_video, m_height_video, m_margin, m_margin + dst_offset, m_margin + client_w, m_margin + dst_offset + dst_h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}
		}
		else
		{
			// center
			int offset_x = (width_wnd - m_width_video) / 2;
			int offset_y = (height_wnd - m_height_video) / 2;
			glBlitFramebuffer(0, 0, m_width_video, m_height_video, offset_x, offset_y, offset_x + m_width_video, offset_y + m_height_video, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
		glBindFramebuffer(GL_READ_FRAMEBUFFER, backbufId);		

		glfwSwapBuffers(m_window);

		return true;
	}

}

