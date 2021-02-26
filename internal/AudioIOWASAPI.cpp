#include "AudioIOWASAPI.h"
#include <functiondiscoverykeys.h>
#include <thread>

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC  10000

namespace LiveKit
{
	inline std::string WSTRtoANSI(const wchar_t* wstr)
	{
		int len = (int)wcslen(wstr);
		if (len == 0) return "";
		int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr, len, nullptr, 0, 0, 0);
		std::vector<char> ret(size_needed + 1, (char)0);
		WideCharToMultiByte(CP_ACP, 0, wstr, len, ret.data(), (int)ret.size(), 0, 0);
		return ret.data();
	}

	const std::vector<std::string>& AudioOut::ImplWASAPI::s_get_list_audio_devices(int* id_default)
	{
		static bool s_first_time = true;
		if (s_first_time)
		{
			CoInitialize(nullptr);
			s_first_time = false;
		}

		static std::vector<std::string> s_list_devices;
		static int s_id_default;
		if (s_list_devices.size() == 0)
		{
			IMMDeviceEnumerator *pEnum;
			CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);

			IMMDevice* pDefaultDevice;
			pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
			LPWSTR wstrID_Default;
			pDefaultDevice->GetId(&wstrID_Default);
			std::wstring str_id_default = wstrID_Default;
			CoTaskMemFree(wstrID_Default);
			pDefaultDevice->Release();

			IMMDeviceCollection *pDevices;
			pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);

			UINT  count;
			pDevices->GetCount(&count);

			for (unsigned i = 0; i < count; i++)
			{
				IMMDevice *pDevice;
				pDevices->Item(i, &pDevice);

				IPropertyStore *pProps;
				pDevice->OpenPropertyStore(STGM_READ, &pProps);

				PROPVARIANT varName;
				PropVariantInit(&varName);
				pProps->GetValue(PKEY_Device_FriendlyName, &varName);
				std::wstring str_name = varName.pwszVal;
				PropVariantClear(&varName);
				pProps->Release();

				s_list_devices.push_back(WSTRtoANSI(str_name.c_str()));

				LPWSTR wstrID = nullptr;
				pDevice->GetId(&wstrID);
				std::wstring str_id = wstrID;
				CoTaskMemFree(wstrID);

				if (str_id == str_id_default)
				{
					s_id_default = i;
				}

				pDevice->Release();
			}

			pDevices->Release();
			pEnum->Release();
		}

		if (id_default != nullptr)
		{
			*id_default = s_id_default;
		}
		return s_list_devices;
	}

	AudioOut::ImplWASAPI::ImplWASAPI(int audio_device_id, int samplerate, AudioWriteCallback callback, EOFCallback eof_callback, void* user_ptr)
		: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
	{
		static bool s_first_time = true;
		if (s_first_time)
		{
			CoInitialize(nullptr);
			s_first_time = false;
		}

		IMMDeviceEnumerator *pEnum;
		CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);

		IMMDeviceCollection *pDevices;
		pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
		pDevices->Item(audio_device_id, &m_pDevice);
		pDevices->Release();
		pEnum->Release();

		m_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_pAudioClient);

		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.nSamplesPerSec = samplerate;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
		format.nBlockAlign = sizeof(short)*format.nChannels;
		format.wBitsPerSample = 16;
		format.cbSize = 0;

		REFERENCE_TIME hnsRequestedDuration = 0;
		m_pAudioClient->GetDevicePeriod(&hnsRequestedDuration, nullptr);

		m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, hnsRequestedDuration, &format, nullptr);

		m_hEvent = CreateEvent(nullptr, FALSE, FALSE, false);
		m_pAudioClient->SetEventHandle(m_hEvent);

		m_pAudioClient->GetBufferSize(&m_bufferFrameCount);
		m_pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_pRenderClient);

		BYTE *pData;
		m_pRenderClient->GetBuffer(m_bufferFrameCount, &pData);
		m_playing = m_callback((short*)pData, m_bufferFrameCount, m_user_ptr);

		DWORD flags = 0;
		m_pRenderClient->ReleaseBuffer(m_bufferFrameCount, flags);

		m_thread_play = (std::unique_ptr<std::thread>)(new std::thread(thread_play, this));

	}

	AudioOut::ImplWASAPI::~ImplWASAPI()
	{
		m_playing = false;
		m_thread_play->join();
		m_thread_play = nullptr;

		m_pRenderClient->Release();
		CloseHandle(m_hEvent);
		m_pAudioClient->Release();
		m_pDevice->Release();
	}

	void AudioOut::ImplWASAPI::thread_play(ImplWASAPI* self)
	{
		self->m_pAudioClient->Start();
		bool eof = false;
		while (self->m_playing)
		{
			DWORD retval = WaitForSingleObject(self->m_hEvent, 2000);
			if (retval != WAIT_OBJECT_0) break;
			if (eof) break;

			UINT32 numFramesPadding;
			self->m_pAudioClient->GetCurrentPadding(&numFramesPadding);

			UINT32 numFramesAvailable = self->m_bufferFrameCount - numFramesPadding;

			BYTE *pData;
			self->m_pRenderClient->GetBuffer(numFramesAvailable, &pData);
			eof = !self->m_callback((short*)pData, numFramesAvailable, self->m_user_ptr);

			DWORD flags = 0;
			self->m_pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
		}
		self->m_pAudioClient->Stop();
		self->m_playing = false;
		self->m_eof_callback(self->m_user_ptr);
	}

	const std::vector<std::string>& AudioIn::ImplWASAPI::s_get_list_audio_devices(int* id_default)
	{
		static bool s_first_time = true;
		if (s_first_time)
		{
			CoInitialize(nullptr);
			s_first_time = false;
		}

		static std::vector<std::string> s_list_devices;
		static int s_id_default;
		if (s_list_devices.size() == 0)
		{
			IMMDeviceEnumerator *pEnum;
			CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);			

			IMMDevice* pDefaultDevice;
			pEnum->GetDefaultAudioEndpoint(eCapture, eConsole, &pDefaultDevice);
			LPWSTR wstrID_Default;
			pDefaultDevice->GetId(&wstrID_Default);
			std::wstring str_id_default = wstrID_Default;
			CoTaskMemFree(wstrID_Default);
			pDefaultDevice->Release();

			IMMDeviceCollection *pDevices;
			pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevices);

			UINT  count;
			pDevices->GetCount(&count);

			for (unsigned i = 0; i < count; i++)
			{
				IMMDevice *pDevice;
				pDevices->Item(i, &pDevice);

				IPropertyStore *pProps;
				pDevice->OpenPropertyStore(STGM_READ, &pProps);

				PROPVARIANT varName;
				PropVariantInit(&varName);
				pProps->GetValue(PKEY_Device_FriendlyName, &varName);
				std::wstring str_name = varName.pwszVal;
				PropVariantClear(&varName);
				pProps->Release();

				s_list_devices.push_back(WSTRtoANSI(str_name.c_str()));

				LPWSTR wstrID = nullptr;
				pDevice->GetId(&wstrID);
				std::wstring str_id = wstrID;
				CoTaskMemFree(wstrID);

				if (str_id == str_id_default)
				{
					s_id_default = i;
				}

				pDevice->Release();
			}

			pDevices->Release();
			pEnum->Release();
		}

		if (id_default != nullptr)
		{
			*id_default = s_id_default;
		}
		return s_list_devices;
	}

	AudioIn::ImplWASAPI::ImplWASAPI(int audio_device_id, int samplerate, AudioReadCallback callback, EOFCallback eof_callback, void* user_ptr)
		: m_callback(callback), m_eof_callback(eof_callback), m_user_ptr(user_ptr)
	{
		static bool s_first_time = true;
		if (s_first_time)
		{
			CoInitialize(nullptr);
			s_first_time = false;
		}

		IMMDeviceEnumerator *pEnum;
		CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);

		IMMDeviceCollection *pDevices;
		pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevices);
		pDevices->Item(audio_device_id, &m_pDevice);
		pDevices->Release();
		pEnum->Release();

		m_pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_pAudioClient);

		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.nSamplesPerSec = samplerate;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * sizeof(short);
		format.nBlockAlign = sizeof(short)*format.nChannels;
		format.wBitsPerSample = 16;
		format.cbSize = 0;

		REFERENCE_TIME hnsRequestedDuration = 0;
		m_pAudioClient->GetDevicePeriod(&hnsRequestedDuration, nullptr);

		m_pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
			hnsRequestedDuration, hnsRequestedDuration, &format, nullptr);

		m_hEvent = CreateEvent(nullptr, FALSE, FALSE, false);
		m_pAudioClient->SetEventHandle(m_hEvent);

		m_pAudioClient->GetBufferSize(&m_bufferFrameCount);
		m_pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_pCaptureClient);

		m_recording = true;
		m_thread_record = (std::unique_ptr<std::thread>)(new std::thread(thread_record, this));
	}

	AudioIn::ImplWASAPI::~ImplWASAPI()
	{
		m_recording = false;
		m_thread_record->join();
		m_thread_record = nullptr;

		m_pCaptureClient->Release();
		CloseHandle(m_hEvent);
		m_pAudioClient->Release();
		m_pDevice->Release();
	}

	void AudioIn::ImplWASAPI::thread_record(ImplWASAPI* self)
	{
		self->m_pAudioClient->Start();
		bool eof = false;
		while (self->m_recording)
		{
			DWORD retval = WaitForSingleObject(self->m_hEvent, 2000);
			if (retval != WAIT_OBJECT_0) break;
			if (eof) break;

			UINT32 packetLength;
			self->m_pCaptureClient->GetNextPacketSize(&packetLength);

			while (packetLength != 0)
			{
				BYTE *pData;
				UINT32 numFramesAvailable;
				DWORD flags;
				self->m_pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
				{
					memset(pData, 0, numFramesAvailable * 2 * sizeof(short));
				}
				eof = !self->m_callback((const short*)pData, numFramesAvailable, self->m_user_ptr);

				self->m_pCaptureClient->ReleaseBuffer(numFramesAvailable);
				self->m_pCaptureClient->GetNextPacketSize(&packetLength);
			}
		}
		self->m_pAudioClient->Stop();
		self->m_recording = false;
		self->m_eof_callback(self->m_user_ptr);
	}
}
