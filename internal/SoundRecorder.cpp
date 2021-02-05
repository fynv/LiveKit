#include <Windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <stdio.h>
#include <string.h>
#include "BufferQueue.h"
#include "SoundRecorder.h"

namespace LiveKit
{
	SoundRecorder::SoundRecorder(int devId, size_t samples_per_buffer) : m_samples_per_buffer(samples_per_buffer)
	{
		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.nSamplesPerSec = 44100;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
		format.nBlockAlign = sizeof(short) * format.nChannels;
		format.wBitsPerSample = 16;
		format.cbSize = 0;

		waveInOpen((LPHWAVEIN)&m_WaveIn, devId, &format, (DWORD_PTR)(s_SoundInCallback), (DWORD_PTR)this, CALLBACK_FUNCTION);

		m_Buffer = new short[samples_per_buffer * format.nChannels * 2];
		m_Buffer1 = m_Buffer;
		m_Buffer2 = m_Buffer + samples_per_buffer * format.nChannels;
		memset(m_Buffer, 0, sizeof(short)*samples_per_buffer * format.nChannels * 2);

		m_WaveHeader1 = (std::unique_ptr<WAVEHDR>)(new WAVEHDR);
		m_WaveHeader1->lpData = (char *)m_Buffer1;
		m_WaveHeader1->dwBufferLength = (DWORD)(samples_per_buffer * format.nChannels * sizeof(short));
		m_WaveHeader1->dwFlags = 0;
		waveInPrepareHeader((HWAVEIN)m_WaveIn, m_WaveHeader1.get(), sizeof(WAVEHDR));
		waveInAddBuffer((HWAVEIN)m_WaveIn, m_WaveHeader1.get(), sizeof(WAVEHDR));

		m_WaveHeader2 = (std::unique_ptr<WAVEHDR>)(new WAVEHDR);
		m_WaveHeader2->lpData = (char *)m_Buffer2;
		m_WaveHeader2->dwBufferLength = (DWORD)(samples_per_buffer* format.nChannels * sizeof(short));
		m_WaveHeader2->dwFlags = 0;	
		waveInPrepareHeader((HWAVEIN)m_WaveIn, m_WaveHeader2.get(), sizeof(WAVEHDR));
		waveInAddBuffer((HWAVEIN)m_WaveIn, m_WaveHeader2.get(), sizeof(WAVEHDR));

		m_buffer_queue = (std::unique_ptr<BufferQueue>)(new BufferQueue);
		m_recycler = (std::unique_ptr<BufferQueue>)(new BufferQueue);

		m_isReceiving = true;
		waveInStart((HWAVEIN)m_WaveIn);
	}

	SoundRecorder::~SoundRecorder()
	{
		m_isReceiving = false;
		waveInStop((HWAVEIN)m_WaveIn);
		waveInReset((HWAVEIN)m_WaveIn);
		waveInUnprepareHeader((HWAVEIN)m_WaveIn, m_WaveHeader1.get(), sizeof(WAVEHDR));
		waveInUnprepareHeader((HWAVEIN)m_WaveIn, m_WaveHeader2.get(), sizeof(WAVEHDR));
		waveInClose((HWAVEIN)m_WaveIn);

		m_buffer_queue = nullptr;
		delete[] m_Buffer;
	}
	
	Buffer* SoundRecorder::get_buffer()
	{
		return m_buffer_queue->PopBuffer();
	}

	void SoundRecorder::recycle_buffer(Buffer* buf)
	{
		m_recycler->PushBuffer(buf);
	}

	void SoundRecorder::s_SoundInCallback(void* hwi, unsigned uMsg, void* dwInstance, void* dwParam1, void* dwParam2)
	{
		if (uMsg == WIM_DATA)
		{
			SoundRecorder* self = (SoundRecorder*)dwInstance;
			WAVEHDR* pwhr = (WAVEHDR*)dwParam1;
			if (!self->m_isReceiving) return;
			self->recordBuf((short*)pwhr->lpData);
			waveInAddBuffer((HWAVEIN)hwi, pwhr, sizeof(WAVEHDR));
		}
	}

	void SoundRecorder::recordBuf(const short* data)
	{
		unsigned size = (unsigned)(m_samples_per_buffer * sizeof(short) * 2);
		Buffer* newBuf;
		if (m_recycler->Size() > 0)
			newBuf = m_recycler->PopBuffer();
		else
			newBuf = new Buffer(m_samples_per_buffer *2);
		memcpy(newBuf->m_data, data, size);
		m_buffer_queue->PushBuffer(newBuf);
	}	

}