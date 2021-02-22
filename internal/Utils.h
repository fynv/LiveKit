#pragma once

#include <cstdint>
#include <chrono>

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */

namespace LiveKit
{

	inline uint64_t time_micro_sec()
	{
		std::chrono::time_point<std::chrono::system_clock> tpSys = std::chrono::system_clock::now();
		std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMicro
			= std::chrono::time_point_cast<std::chrono::microseconds>(tpSys);
		return tpMicro.time_since_epoch().count();
	}

	inline double time_sec()
	{
		return (double)time_micro_sec() / 1000000.0;
	}

	inline bool exists_test(const char* name)
	{
		if (FILE *file = fopen(name, "r"))
		{
			fclose(file);
			return true;
		}
		else
		{
			return false;
		}
	}

	inline void copy_centered_samechn(const uint8_t* data_in, int width_in, int height_in,
		uint8_t* data_out, int width_out, int height_out, int chn, bool flip)
	{
		int offset_in_x = 0;
		int offset_in_y = 0;
		int offset_out_x = 0;
		int offset_out_y = 0;
		int scan_w = min(width_out, width_in);
		int scan_h = min(height_out, height_in);

		if (width_in < width_out)
			offset_out_x = (width_out - width_in) / 2;
		else if (width_in > width_out)
			offset_in_x = (width_in - width_out) / 2;

		if (height_in < height_out)
			offset_out_y = (height_out - height_in) / 2;
		else if (height_in > height_out)
			offset_in_y = (height_in - height_out) / 2;

		for (int y = 0; y < scan_h; y++)
		{
			const uint8_t* p_in_line;
			if (!flip)
			{
				p_in_line = data_in + (y + offset_in_y)*width_in*chn;
			}
			else
			{
				int y2 = (scan_h - 1) - y;
				p_in_line = data_in + (y2 + offset_in_y)*width_in*chn;
			}

			uint8_t* p_out_line = data_out + (y + offset_out_y)*width_out * chn;

			const uint8_t* p_in = p_in_line + offset_in_x*chn;
			uint8_t* p_out = p_out_line + offset_out_x * chn;
			memcpy(p_out, p_in, scan_w*chn);
		}
	}

	inline void copy_centered(const uint8_t* data_in, int width_in, int height_in, int chn_in,
		uint8_t* data_out, int width_out, int height_out, int chn_out, bool flip)
	{
		memset(data_out, 0, width_out*height_out*chn_out);

		if (chn_in == chn_out)
			return copy_centered_samechn(data_in, width_in, height_in, data_out, width_out, height_out, chn_in, flip);		

		int offset_in_x = 0;
		int offset_in_y = 0;
		int offset_out_x = 0;
		int offset_out_y = 0;
		int scan_w = min(width_out, width_in);
		int scan_h = min(height_out, height_in);

		if (width_in < width_out)
			offset_out_x = (width_out - width_in) / 2;
		else if (width_in > width_out)
			offset_in_x = (width_in - width_out) / 2;

		if (height_in < height_out)
			offset_out_y = (height_out - height_in) / 2;
		else if (height_in > height_out)
			offset_in_y = (height_in - height_out) / 2;

		for (int y = 0; y < scan_h; y++)
		{
			const uint8_t* p_in_line;
			if (!flip)
			{
				p_in_line = data_in + (y + offset_in_y)*width_in*chn_in;
			}
			else
			{
				int y2 = (scan_h - 1) - y;
				p_in_line = data_in + (y2 + offset_in_y)*width_in*chn_in;
			}

			uint8_t* p_out_line = data_out + (y + offset_out_y)*width_out * chn_out;
			for (int x = 0; x < scan_w; x++)
			{
				const uint8_t* p_in = p_in_line + (x + offset_in_x)*chn_in;
				uint8_t* p_out = p_out_line + (x + offset_out_x) * chn_out;
				int c = 0;
				for (; c<min(chn_in, chn_out); c++)
					p_out[c] = p_in[c];
				for (; c < chn_out; c++)
					p_out[c] = 255;
			}
		}
	}

}