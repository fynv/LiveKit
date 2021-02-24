#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <Windows.h>

namespace LiveKit
{
	typedef bool (*AudioWriteCallback)(short* buf, void* user_ptr);
	typedef bool (*AudioReadCallback)(const short* buf, void* user_ptr);
	typedef void (*EOFCallback)(void* user_ptr);

	class AudioPort
	{
	public:
		AudioPort();
		~AudioPort();

		int AddWriter(int samplerate, int samples_per_buffer, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr);
		void RemoveWriter(int id);
		void SetVolume(int id, float vol);

		int AddReader(int samplerate, int samples_per_buffer, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr);
		void RemoveReader(int id);

		bool IsIdling() const
		{
			return m_writers.size() == 0 || m_readers.size() == 0;
		}


	private:
		class Writer;
		class Reader;		

		uint64_t m_ref_t;
		
		int m_id_next_writer = 0;
		std::unordered_map<int, std::unique_ptr<Writer>> m_writers;
		mutable CRITICAL_SECTION m_cs_writers;

		int m_id_next_reader = 0;
		std::unordered_map<int, std::unique_ptr<Reader>> m_readers;

		void _read(double t_read, int samplerate, float* buf, size_t size) const;
		

	};


}