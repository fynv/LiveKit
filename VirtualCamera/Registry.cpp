#include "Registry.h"
#include <windows.h>

std::string MAPPING_NAME = "LiveKitVCam";
int VIDEO_WIDTH = 640;
int VIDEO_HEIGHT = 480;

void LoadRegistry()
{
	HKEY hKey;
	RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\LiveKitVCam\\Video", 0, KEY_READ, &hKey);
	DWORD width, height;

	char mapping_name[1024];
	DWORD dwBufferSize = 1024;
	if (RegQueryValueExA(hKey, "mapping_name", nullptr, nullptr, (LPBYTE)mapping_name, &dwBufferSize) == ERROR_SUCCESS)
		MAPPING_NAME = mapping_name;

	DWORD size = 4;
	if (RegQueryValueExA(hKey, "width", nullptr, nullptr, (LPBYTE)&width, &size) == ERROR_SUCCESS)
		VIDEO_WIDTH = (int)width;

	if (RegQueryValueExA(hKey, "height", nullptr, nullptr, (LPBYTE)&height, &size) == ERROR_SUCCESS)
		VIDEO_HEIGHT = (int)height;

	RegCloseKey(hKey);
}


