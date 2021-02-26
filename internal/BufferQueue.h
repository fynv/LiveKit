#pragma once

#include "AudioBuffer.h"
#include <queue>
#include <Windows.h>

namespace LiveKit
{
	class BufferQueue
	{
	public:
		BufferQueue()
		{
			InitializeCriticalSectionAndSpinCount(&m_cs, 0x00000400);
			m_hSemaphore = CreateSemaphore(NULL, 0, ~(1 << 31), NULL);
		}

		~BufferQueue()
		{
			while (m_queue.size() > 0)
			{
				AudioBuffer* buf = PopBuffer();
				delete buf;
			}
			CloseHandle(m_hSemaphore);
			DeleteCriticalSection(&m_cs);
		}

		size_t Size()
		{
			return m_queue.size();
		}

		void PushBuffer(AudioBuffer* buf)
		{
			EnterCriticalSection(&m_cs);
			m_queue.push(buf);
			LeaveCriticalSection(&m_cs);
			ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}

		AudioBuffer* PopBuffer()
		{
			WaitForSingleObject(m_hSemaphore, INFINITE);
			EnterCriticalSection(&m_cs);
			AudioBuffer* ret = m_queue.front();
			m_queue.pop();
			LeaveCriticalSection(&m_cs);
			return ret;
		}

	private:
		std::queue<AudioBuffer*> m_queue;
		CRITICAL_SECTION m_cs;
		HANDLE m_hSemaphore;
	};

}