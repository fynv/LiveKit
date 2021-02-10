#pragma once

#include "VideoPort.h"
#include <memory>

namespace LiveKit
{
	class Image;

	class LazyPlayer : public VideoSource
	{
	public:
		LazyPlayer(const char* fn);
		~LazyPlayer();

		int video_width() const;
		int video_height() const;

		bool is_playing() const { return m_is_playing; }
		bool is_eof_reached() const;
		uint64_t get_duration() const;
		uint64_t get_position() const;
		
		void stop();
		void start();
		void set_position(uint64_t pos);
		
		virtual const Image* read_image(uint64_t* timestamp) const;

	private:
		class Internal;		
		std::unique_ptr<Internal> m_internal;
		bool m_is_playing = false;
		uint64_t m_start_pos = 0;
		uint64_t m_start_time = 0;
		mutable uint64_t m_timestamp = 0;
	};

}
