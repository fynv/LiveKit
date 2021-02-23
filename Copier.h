#pragma once

#include <memory>
#include <vector>

namespace std
{
	class thread;
}

struct AVFormatContext;

namespace LiveKit
{
	class Copier
	{
	public:
		Copier(const char* filename_in, const char* filename_out);
		~Copier();

		bool IsCopying() const { return m_copying; }


	private:
		AVFormatContext* m_p_fmt_ctx_in = nullptr;
		AVFormatContext* m_p_fmt_ctx_out = nullptr;
		std::vector<int> m_stream_mapping;

		bool m_copying = false;
		static void thread_copy(Copier* self);
		std::unique_ptr<std::thread> m_thread_copy;
	};

}

