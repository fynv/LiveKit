#include "Viewer.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Image.h"
#include "VideoPort.h"
#include "RenderingOGL.h"

namespace LiveKit
{

	Viewer::Viewer(int window_width, int window_height, const char* title)
	{
		glfwInit();
		m_window = glfwCreateWindow(window_width, window_height, title, NULL, NULL);
		glfwMakeContextCurrent(m_window);
		glewInit();
		glGenTextures(1, &m_tex_id);
	}

	Viewer::~Viewer()
	{
		glDeleteTextures(1, &m_tex_id);
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	bool Viewer::Draw()
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(m_window)) return false;

		if (m_source != nullptr)
		{
			uint64_t new_timestamp;
			const Image* image = m_source->read_image(&new_timestamp);
			if (new_timestamp!=(uint64_t)(-1) && new_timestamp != m_timestamp)
			{			
				m_timestamp = new_timestamp;

				glBindTexture(GL_TEXTURE_2D, m_tex_id);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				if (image->has_alpha())
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width(), image->height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, image->data());
				}
				else
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->width(), image->height(), 0, GL_BGR, GL_UNSIGNED_BYTE, image->data());
				}
				glBindTexture(GL_TEXTURE_2D, 0);

				m_flipped = image->is_flipped();
			}
		}


		int vw, vh;
		glfwGetFramebufferSize(m_window, &vw, &vh);
		glViewport(0, 0, vw, vh);		
		DrawFullScreen(m_tex_id, m_flipped);
		glfwSwapBuffers(m_window);

		return true;
	}


}
