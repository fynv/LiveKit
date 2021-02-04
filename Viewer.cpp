#include "Viewer.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Image.h"
#include "VideoPort.h"

namespace LiveKit
{

	struct GLShader
	{
		unsigned m_type = 0;
		unsigned m_id = -1;
		GLShader(unsigned type, const char* code);
		~GLShader();
	};


	GLShader::GLShader(unsigned type, const char* code)
	{
		m_type = type;
		m_id = glCreateShader(type);
		glShaderSource(m_id, 1, &code, nullptr);
		glCompileShader(m_id);

		GLint compileResult;
		glGetShaderiv(m_id, GL_COMPILE_STATUS, &compileResult);
		if (compileResult == 0)
		{
			GLint infoLogLength;
			glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
			std::vector<GLchar> infoLog(infoLogLength);
			glGetShaderInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

			printf("Shader compilation failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
		}
	}


	GLShader::~GLShader()
	{
		if (m_id != -1)
			glDeleteShader(m_id);
	}


	struct GLProgram
	{
		unsigned m_id = -1;
		GLProgram(const GLShader& vertexShader, const GLShader& fragmentShader);
		GLProgram(const GLShader& computeShader);
		~GLProgram();
	};


	GLProgram::GLProgram(const GLShader& vertexShader, const GLShader& fragmentShader)
	{
		m_id = glCreateProgram();
		glAttachShader(m_id, vertexShader.m_id);
		glAttachShader(m_id, fragmentShader.m_id);
		glLinkProgram(m_id);

		GLint linkResult;
		glGetProgramiv(m_id, GL_LINK_STATUS, &linkResult);
		if (linkResult == 0)
		{
			GLint infoLogLength;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
			std::vector<GLchar> infoLog(infoLogLength);
			glGetProgramInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

			printf("Shader link failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
		}
	}

	GLProgram::GLProgram(const GLShader& computeShader)
	{
		m_id = glCreateProgram();
		glAttachShader(m_id, computeShader.m_id);
		glLinkProgram(m_id);

		GLint linkResult;
		glGetProgramiv(m_id, GL_LINK_STATUS, &linkResult);
		if (linkResult == 0)
		{
			GLint infoLogLength;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
			std::vector<GLchar> infoLog(infoLogLength);
			glGetProgramInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

			printf("Shader link failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
		}
	}

	GLProgram::~GLProgram()
	{
		if (m_id != -1)
			glDeleteProgram(m_id);
	}


	static const char* g_vertex_shader_fullscreen =
		"#version 430\n"
		"layout (location = 0) out vec2 vUV;\n"
		"void main()\n"
		"{\n"
		"    vec2 grid = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
		"    vec2 vpos = grid * vec2(2.0, 2.0) + vec2(-1.0, -1.0);\n"
		"    gl_Position = vec4(vpos, 1.0, 1.0);\n"
		"    vUV = vec2(grid.x, 1.0 - grid.y);\n"
		"}\n";

	static const char* g_vertex_shader_fullscreen_flipped =
		"#version 430\n"
		"layout (location = 0) out vec2 vUV;\n"
		"void main()\n"
		"{\n"
		"    vec2 grid = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
		"    vec2 vpos = grid * vec2(2.0, 2.0) + vec2(-1.0, -1.0);\n"
		"    gl_Position = vec4(vpos, 1.0, 1.0);\n"
		"    vUV = vec2(grid.x, grid.y);\n"
		"}\n";

	static const char* g_frag_shader_fullscreen =
		"#version 430\n"
		"layout (location = 0) in vec2 vUV;\n"
		"layout (location = 0) out vec4 outColor;\n"
		"layout (location = 0) uniform sampler2D uTex;\n"
		"void main()\n"
		"{\n"
		"	outColor = texture(uTex, vUV);\n"
		"}\n";


	void DrawFullScreen(unsigned tex_id, bool flipped)
	{
		static GLShader s_vertex_shader_fullscreen(GL_VERTEX_SHADER, g_vertex_shader_fullscreen);
		static GLShader s_vertex_shader_fullscreen_flipped(GL_VERTEX_SHADER, g_vertex_shader_fullscreen_flipped);
		static GLShader s_frag_shader_fullscreen(GL_FRAGMENT_SHADER, g_frag_shader_fullscreen);
		static GLProgram s_program_fullscreen(s_vertex_shader_fullscreen, s_frag_shader_fullscreen);
		static GLProgram s_program_fullscreen_flipped(s_vertex_shader_fullscreen_flipped, s_frag_shader_fullscreen);

		glDisable(GL_DEPTH_TEST);
		glUseProgram(flipped? s_program_fullscreen_flipped.m_id : s_program_fullscreen.m_id);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		glUniform1i(0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}

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
