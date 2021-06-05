#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include "VideoFilter.h"
#include "ImageFetch.h"
#include <deque>
#include <cstdint>
#include "clock.h"

struct Format
{
	Format(int width_, int height_, int64_t time_per_frame_) {
		width = width_;
		height = height_;
		time_per_frame = time_per_frame_;
	}
	int width;
	int height;
	int64_t time_per_frame;
};

EXTERN_C const GUID CLSID_VirtualCam;

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:
	CVCamStream(HRESULT *phr, CVideoFilter *pParent, LPCWSTR pPinName) :
		CSourceStream(NAME("Video"), phr, pParent, pPinName), m_parent(pParent)
	{
		ListSupportFormat();	
		GetMediaType(0, &m_mt);
	}

	~CVCamStream()
	{

	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
	{
		if (riid == _uuidof(IAMStreamConfig))
			*ppv = (IAMStreamConfig*)this;
		else if (riid == _uuidof(IKsPropertySet))
			*ppv = (IKsPropertySet*)this;
		else
			return CSourceStream::QueryInterface(riid, ppv);

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt)
	{
		if (pmt == nullptr)
			return E_FAIL; 

		if (m_parent->GetState() != State_Stopped)
			return E_FAIL;

		if (CheckMediaType((CMediaType *)pmt) != S_OK)
			return E_FAIL;

		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pmt->pbFormat);
		m_mt.SetFormat(m_mt.Format(), sizeof(VIDEOINFOHEADER));
		m_format_list.push_front({ pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight, pvi->AvgTimePerFrame });

		IPin* pin;
		ConnectedTo(&pin);
		if (pin){
			IFilterGraph *pGraph = m_parent->GetGraph();
			pGraph->Reconnect(this);
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt)
	{
		*ppmt = CreateMediaType(&m_mt);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize)
	{
		if (m_format_list.size() == 0)
			ListSupportFormat();

		*piCount = m_format_list.size();
		*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
	{
		if (m_format_list.size() == 0)
			ListSupportFormat();

		if (iIndex < 0 || iIndex > (int)m_format_list.size() - 1)
			return E_INVALIDARG;

		*pmt = CreateMediaType(&m_mt);
		DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

		pvi->bmiHeader.biWidth = m_format_list[iIndex].width;
		pvi->bmiHeader.biHeight = m_format_list[iIndex].height;
		pvi->AvgTimePerFrame = m_format_list[iIndex].time_per_frame;
		pvi->AvgTimePerFrame = 333333;
		pvi->bmiHeader.biCompression = BI_RGB;
		pvi->bmiHeader.biBitCount = 24;
		pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biPlanes = 1;
		pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
		pvi->bmiHeader.biClrImportant = 0;

		SetRectEmpty(&(pvi->rcSource));
		SetRectEmpty(&(pvi->rcTarget));

		(*pmt)->majortype = MEDIATYPE_Video;
		(*pmt)->subtype = MEDIASUBTYPE_RGB24;
		(*pmt)->formattype = FORMAT_VideoInfo;
		(*pmt)->bTemporalCompression = FALSE;
		(*pmt)->bFixedSizeSamples = FALSE;
		(*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
		(*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

		DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
		pvscc->guid = FORMAT_VideoInfo;
		pvscc->VideoStandard = AnalogVideo_None;
		pvscc->InputSize.cx = pvi->bmiHeader.biWidth;
		pvscc->InputSize.cy = pvi->bmiHeader.biHeight;
		pvscc->MinCroppingSize.cx = pvi->bmiHeader.biWidth;
		pvscc->MinCroppingSize.cy = pvi->bmiHeader.biHeight;
		pvscc->MaxCroppingSize.cx = pvi->bmiHeader.biWidth;
		pvscc->MaxCroppingSize.cy = pvi->bmiHeader.biHeight;
		pvscc->CropGranularityX = pvi->bmiHeader.biWidth;
		pvscc->CropGranularityY = pvi->bmiHeader.biHeight;
		pvscc->CropAlignX = 0;
		pvscc->CropAlignY = 0;

		pvscc->MinOutputSize.cx = pvi->bmiHeader.biWidth;
		pvscc->MinOutputSize.cy = pvi->bmiHeader.biHeight;
		pvscc->MaxOutputSize.cx = pvi->bmiHeader.biWidth;
		pvscc->MaxOutputSize.cy = pvi->bmiHeader.biHeight;

		pvscc->OutputGranularityX = 0;
		pvscc->OutputGranularityY = 0;
		pvscc->StretchTapsX = 0;
		pvscc->StretchTapsY = 0;
		pvscc->ShrinkTapsX = 0;
		pvscc->ShrinkTapsY = 0;
		pvscc->MinFrameInterval = pvi->AvgTimePerFrame;
		pvscc->MaxFrameInterval = pvi->AvgTimePerFrame;
		pvscc->MinBitsPerSecond = pvi->bmiHeader.biWidth * pvi->bmiHeader.biHeight
			* 2 * 8 * (10000000 / pvi->AvgTimePerFrame);
		pvscc->MaxBitsPerSecond = pvi->bmiHeader.biWidth * pvi->bmiHeader.biHeight
			* 2 * 8 * (10000000 / pvi->AvgTimePerFrame);

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID,
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID,
		void *pInstanceData, DWORD cbInstanceData, void *pPropData,
		DWORD cbPropData, DWORD *pcbReturned)
	{
		if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
		if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
		if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

		if (pcbReturned) *pcbReturned = sizeof(GUID);
		if (pPropData == NULL)          return S_OK;
		if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;

		*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
	{
		if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
		if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
		if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
		return S_OK;
	}

	HRESULT FillBuffer(IMediaSample *pms)
	{
		HRESULT hr;
		uint8_t* dst;
		hr = pms->GetPointer((BYTE**)&dst);
		long len = pms->GetActualDataLength();

		if (m_system_start_time <= 0)
			m_system_start_time = get_current_time();

		REFERENCE_TIME wait_start_time = get_current_time(m_system_start_time);
		while (get_current_time(m_system_start_time) - wait_start_time < ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame * 2)
		{
			if (FetchImage(dst))
			{
				break;
			}
			else
			{
				Sleep(5);
			}
		}
		
		REFERENCE_TIME start_time = get_current_time(m_system_start_time);
		REFERENCE_TIME end_time = start_time + ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

		pms->SetTime(&start_time, &end_time);
		pms->SetSyncPoint(TRUE);

		return NOERROR;
	}

	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
	{
		CAutoLock cAutoLock(m_pFilter->pStateLock());
		HRESULT hr = NOERROR;

		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
		pProperties->cBuffers = 1;
		pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

		ALLOCATOR_PROPERTIES Actual;
		hr = pAlloc->SetProperties(pProperties, &Actual);

		if (FAILED(hr)) return hr;
		if (Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

		return NOERROR;
	}

	HRESULT CheckMediaType(const CMediaType *pMediaType)
	{
		if (pMediaType == nullptr)
			return E_FAIL;

		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());

		const GUID* type = pMediaType->Type();
		const GUID* info = pMediaType->FormatType();
		const GUID* subtype = pMediaType->Subtype();

		if (*type != MEDIATYPE_Video)
			return E_INVALIDARG;

		if (*info != FORMAT_VideoInfo)
			return E_INVALIDARG;

		if (*subtype != MEDIASUBTYPE_RGB24)
			return E_INVALIDARG;

		if (pvi->AvgTimePerFrame < 166666 || pvi->AvgTimePerFrame >1000000)
			return E_INVALIDARG;

		if (ValidateResolution(pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight))
			return S_OK;

		return E_INVALIDARG;
	}

	HRESULT GetMediaType(int iPosition, CMediaType *pmt)
	{
		if (m_format_list.size() == 0)
			ListSupportFormat();

		if (iPosition < 0 || iPosition > (int)m_format_list.size() - 1)
			return E_INVALIDARG;

		DECLARE_PTR(VIDEOINFOHEADER, pvi,
		pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
		ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));


		pvi->bmiHeader.biWidth = m_format_list[iPosition].width;
		pvi->bmiHeader.biHeight = m_format_list[iPosition].height;
		pvi->AvgTimePerFrame = m_format_list[iPosition].time_per_frame;
		pvi->bmiHeader.biCompression = BI_RGB;
		pvi->bmiHeader.biBitCount = 24;
		pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biPlanes = 1;
		pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
		pvi->bmiHeader.biClrImportant = 0;

		SetRectEmpty(&(pvi->rcSource));
		SetRectEmpty(&(pvi->rcTarget));

		pmt->SetType(&MEDIATYPE_Video);
		pmt->SetFormatType(&FORMAT_VideoInfo);
		pmt->SetTemporalCompression(FALSE);
		pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
		pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
		return NOERROR;
	}

	HRESULT SetMediaType(const CMediaType *pmt)
	{
		DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
		HRESULT hr = CSourceStream::SetMediaType(pmt);
		return hr;
	}

	HRESULT OnThreadCreate(void)
	{
		return NOERROR;
	}

	HRESULT OnThreadDestroy(void)
	{
		return NOERROR;
	}


private:
	bool ListSupportFormat(void)
	{
		if (m_format_list.size() > 0)
			m_format_list.empty();

		m_format_list.push_back({ VIDEO_WIDTH, VIDEO_HEIGHT, 333333 });

		return true;
	}
	bool ValidateResolution(long width, long height)
	{
		if (width < 320 || height < 240)
			return false;
		else if (width > 4096 || height >4096)
			return false;
		else if (width * 9 == height * 16)
			return true;
		else if (width * 3 == height * 4)
			return true;
		else if (width * 16 == height * 9)
			return true;
		else if (width * 4 == height * 3)
			return true;
		else
			return false;
	}

	CVideoFilter *m_parent;
	std::deque<Format> m_format_list;

	uint64_t m_system_start_time = 0;
};

CUnknown * WINAPI CVideoFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	//::AllocConsole();
	//freopen("CONOUT$", "w+", stdout);

	ASSERT(phr);
	LoadRegistry();
	CUnknown *punk = new CVideoFilter(lpunk, phr);
	return punk;
}

CVideoFilter::CVideoFilter(LPUNKNOWN lpunk, HRESULT *phr) :
	CSource(NAME("LiveKit Virtual Camera"), lpunk, CLSID_VirtualCam)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);

	// Create the one and only output pin
	m_paStreams = (CSourceStream **) new CVCamStream*[1];
	m_paStreams[0] = new CVCamStream(phr, this, L"Virtual Cam");
}


