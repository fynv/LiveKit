#pragma once

#include <Windows.h>
#include <queue>

namespace LiveKit
{
	class Buffer
	{
	public:
		short* m_data;
		Buffer(size_t size)
		{
			m_data = new short[size];
		}
		~Buffer()
		{
			delete[] m_data;
		}
	};

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
				Buffer* buf = PopBuffer();
				delete buf;
			}
			CloseHandle(m_hSemaphore);
			DeleteCriticalSection(&m_cs);
		}

		size_t Size()
		{
			return m_queue.size();
		}

		void PushBuffer(Buffer* buf)
		{
			EnterCriticalSection(&m_cs);
			m_queue.push(buf);
			LeaveCriticalSection(&m_cs);
			ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}

		Buffer* PopBuffer()
		{
			WaitForSingleObject(m_hSemaphore, INFINITE);
			EnterCriticalSection(&m_cs);
			Buffer* ret = m_queue.front();
			m_queue.pop();
			LeaveCriticalSection(&m_cs);
			return ret;
		}

	private:
		std::queue<Buffer*> m_queue;
		CRITICAL_SECTION m_cs;
		HANDLE m_hSemaphore;
	};

}

