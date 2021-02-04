#pragma once

#include <memory>
struct wavehdr_tag;
namespace LiveKit
{
	class Buffer;
	class BufferQueue;
	class SoundRecorder
	{
	public:
		SoundRecorder(int devId, size_t samples_per_buffer);
		~SoundRecorder();

		Buffer* get_buffer();
		void recycle_buffer(Buffer* buf);


	private:
		size_t m_samples_per_buffer;
		std::unique_ptr<BufferQueue> m_buffer_queue;
		std::unique_ptr<BufferQueue> m_recycler;

		bool m_isReceiving = false;
		void* m_WaveIn;
		short  *m_Buffer;
		short  *m_Buffer1, *m_Buffer2;
		std::unique_ptr<wavehdr_tag> m_WaveHeader1;
		std::unique_ptr<wavehdr_tag> m_WaveHeader2;

		static void __stdcall s_SoundInCallback(void* hwi, unsigned uMsg, void* dwInstance, void* dwParam1, void* dwParam2);
		void recordBuf(const short* data);

	};
}

