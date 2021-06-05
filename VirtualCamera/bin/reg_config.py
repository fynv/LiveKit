import winreg

MAPPING_NAME = "LiveKitVCam"
VIDEO_WIDTH = 1200
VIDEO_HEIGHT = 900

key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, "Software\\LiveKitVCam\\Video")
winreg.SetValueEx(key, "mapping_name", 0, winreg.REG_SZ, MAPPING_NAME)
winreg.SetValueEx(key, "width", 0, winreg.REG_DWORD, VIDEO_WIDTH)
winreg.SetValueEx(key, "height", 0, winreg.REG_DWORD, VIDEO_HEIGHT)



