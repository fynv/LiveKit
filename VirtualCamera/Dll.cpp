#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")

#ifdef _DEBUG
#pragma comment(lib, "strmbasd")
#else
#pragma comment(lib, "strmbase")
#endif

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <dllsetup.h>
#include "VideoFilter.h"

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);


STDAPI AMovieSetupRegisterServer(CLSID clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);

DEFINE_GUID(CLSID_VirtualCam,
	0x8e14549a, 0xdb61, 0x4309, 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x30);


const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCam =
{
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_RGB24
};

const AMOVIESETUP_PIN AMSPinVCam =
{
	L"Output",             // Pin string name
	FALSE,                 // Is it rendered
	TRUE,                  // Is it an output
	FALSE,                 // Can we have none
	FALSE,                 // Can we have many
	&CLSID_NULL,           // Connects to filter
	NULL,                  // Connects to pin
	1,                     // Number of types
	&AMSMediaTypesVCam      // Pin Media types
};


const AMOVIESETUP_FILTER AMSFilterVCam =
{
	&CLSID_VirtualCam,  // Filter CLSID
	L"LiveKit Virtual Camera",     // String name
	MERIT_DO_NOT_USE,      // Filter merit
	1,                     // Number pins
	&AMSPinVCam             // Pin details
};

CFactoryTemplate g_Templates[] =
{
	{
		L"LiveKit Virtual Camera",
		&CLSID_VirtualCam,
		CVideoFilter::CreateInstance,
		NULL,
		&AMSFilterVCam
	}
};

int g_cTemplates = 1;


STDAPI RegisterFilters(BOOL bRegister)
{
	HRESULT hr = NOERROR;
	WCHAR achFileName[MAX_PATH];
	char achTemp[MAX_PATH];
	ASSERT(g_hInst != 0);

	if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
		return AmHresultFromWin32(GetLastError());

	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
		achFileName, NUMELMS(achFileName));

	hr = CoInitialize(0);
	if (bRegister)
	{
		hr = AMovieSetupRegisterServer(CLSID_VirtualCam, g_Templates[0].m_Name, achFileName);
	}

	if (SUCCEEDED(hr))
	{
		IFilterMapper2 *fm = 0;
		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);
		if (SUCCEEDED(hr))
		{
			if (bRegister)
			{
				IMoniker *pMoniker_video = 0;
				REGFILTER2 rf2;
				rf2.dwVersion = 1;
				rf2.dwMerit = MERIT_DO_NOT_USE;
				rf2.cPins = 1;
				rf2.rgPins = &AMSPinVCam;
				hr = fm->RegisterFilter(CLSID_VirtualCam, g_Templates[0].m_Name, &pMoniker_video, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
			}
			else
			{
				hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_VirtualCam);
			}
		}

		// release interface
		//
		if (fm)
			fm->Release();
	}

	if (SUCCEEDED(hr) && !bRegister)
	{
		hr = AMovieSetupUnregisterServer(CLSID_VirtualCam);
	}

	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr;
}



STDAPI DllInstall(BOOL bInstall, _In_opt_ LPCWSTR pszCmdLine)
{
	if (!bInstall)
		return RegisterFilters(FALSE);
	else
		return RegisterFilters(TRUE);
}

STDAPI DllRegisterServer()
{
	return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
	return RegisterFilters(FALSE);
}


extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

