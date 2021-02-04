#include <stdio.h>
#include <WindowCapture.h>
#include <Recorder.h>
using namespace LiveKit;

#include <string>
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>

const std::vector<std::string>* GetAudioInputDeviceList()
{
	static std::vector<std::string> s_devices;
	if (s_devices.size() == 0)
	{
		unsigned num_dev = waveInGetNumDevs();
		WAVEINCAPS waveInDevCaps;
		for (unsigned i = 0; i < num_dev; i++)
		{
			waveInGetDevCaps(i, &waveInDevCaps, sizeof(WAVEINCAPS));
			s_devices.push_back(waveInDevCaps.szPname);
		}
	}
	return &s_devices;
}


int main()
{
	int idx_win = -1;
	const std::vector<std::string>* lst_windows = WindowCapture::s_get_window_titles();
	for (size_t i = 0; i < lst_windows->size(); i++)
	{
		if ((*lst_windows)[i].find("Ò¹Éñ") != std::string::npos)
		{
			idx_win = (int)i;
			break;
		}
	}

	int idx_audio = -1;
	const std::vector<std::string>* lst_audio_devices = GetAudioInputDeviceList();
	for (size_t i = 0; i < lst_audio_devices->size(); i++)
	{
		const std::string& dev_name = (*lst_audio_devices)[i];
		if (dev_name.length() > 5 && dev_name.substr(0, 5) == "CABLE")
		{
			idx_audio = i;
			break;
		}
	}

	if (idx_win >= 0)
	{
		WindowCapture wc(idx_win);
		Recorder recorder("test.mp4", true, 576, 1024, idx_audio);
		recorder.SetSource(&wc);
		recorder.start();
		printf("Press enter to quit\n");
		std::string user_input;
		std::getline(std::cin, user_input);
	}


	return 0;
}

