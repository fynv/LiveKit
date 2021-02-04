#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define PY_LiveKit_API __declspec(dllexport)
#else
#define PY_LiveKit_API 
#endif

extern "C"
{
	PY_LiveKit_API void StrListDestroy(void* ptr);
	PY_LiveKit_API unsigned long long StrListSize(void* ptr);
	PY_LiveKit_API const char* StrListGet(void* ptr, int idx);

	PY_LiveKit_API void* VideoPortCreate();
	PY_LiveKit_API void VideoPortDestroy(void* ptr);
	PY_LiveKit_API void* VideoPortGetSourcePtr(void* ptr);
	PY_LiveKit_API void* VideoPortGetTargetPtr(void* ptr);

	PY_LiveKit_API void* ImageFileCreate(const char* filename);
	PY_LiveKit_API void ImageFileDestroy(void* ptr);
	PY_LiveKit_API void* ImageFileGetSourcePtr(void* ptr);
	PY_LiveKit_API int ImageFileWidth(void* ptr);
	PY_LiveKit_API int ImageFileHeight(void* ptr);

	PY_LiveKit_API void* CameraListCreate();

	PY_LiveKit_API void* CameraCreate(int idx);
	PY_LiveKit_API void CameraDestroy(void* ptr);
	PY_LiveKit_API int CameraIdx(void* ptr);
	PY_LiveKit_API int CameraWidth(void* ptr);
	PY_LiveKit_API int CameraHeight(void* ptr);
	PY_LiveKit_API void CameraAddTarget(void* ptr, void* p_target);

	PY_LiveKit_API void* ViewerCreate(int window_width, int window_height, const char* title);
	PY_LiveKit_API void ViewerDestroy(void* ptr);
	PY_LiveKit_API void ViewerSetSource(void* ptr, void* p_source);
	PY_LiveKit_API int ViewerDraw(void* ptr);

	PY_LiveKit_API void* WindowListCreate();

	PY_LiveKit_API void* WindowCaptureCreate(int idx);
	PY_LiveKit_API void WindowCaptureDestroy(void* ptr);
	PY_LiveKit_API void* WindowCaptureGetSourcePtr(void* ptr);

	PY_LiveKit_API void* AudioInputDeviceListCreate();

	PY_LiveKit_API void* RecorderCreate(const char* filename, int mp4, int video_width, int video_height, int audio_device_id);
	PY_LiveKit_API void RecorderDestroy(void* ptr);
	PY_LiveKit_API void RecorderSetSource(void* ptr, void* p_source);
	PY_LiveKit_API void RecorderStart(void* ptr);
	PY_LiveKit_API void RecorderStop(void* ptr);

}

#include <VideoPort.h>
#include <ImageFile.h>
#include <Camera.h>
#include <Viewer.h>
#include <WindowCapture.h>
#include <Recorder.h>
using namespace LiveKit;

#include <vector>
#include <string>

void StrListDestroy(void* ptr)
{
	std::vector<std::string>* lst = (std::vector<std::string>*)ptr;
	delete lst;
}

unsigned long long StrListSize(void* ptr)
{
	std::vector<std::string>* lst = (std::vector<std::string>*)ptr;
	return lst->size();
}

const char* StrListGet(void* ptr, int idx)
{
	std::vector<std::string>* lst = (std::vector<std::string>*)ptr;
	return (*lst)[idx].c_str();
}

void* VideoPortCreate()
{
	return new VideoPort;
}

void VideoPortDestroy(void* ptr)
{
	VideoPort* port = (VideoPort*)ptr;
	delete port;
}

void* VideoPortGetSourcePtr(void* ptr)
{
	VideoPort* port = (VideoPort*)ptr;
	return (VideoSource*)port;
}

void* VideoPortGetTargetPtr(void* ptr)
{
	VideoPort* port = (VideoPort*)ptr;
	return (VideoTarget*)port;
}


void* ImageFileCreate(const char* filename)
{
	return new ImageFile(filename);
}

void ImageFileDestroy(void* ptr)
{
	ImageFile* imgfile = (ImageFile*)ptr;
	delete imgfile;
}

void* ImageFileGetSourcePtr(void* ptr)
{
	ImageFile* imgfile = (ImageFile*)ptr;
	return (VideoSource*)imgfile;
}


int ImageFileWidth(void* ptr)
{
	ImageFile* imgfile = (ImageFile*)ptr;
	return imgfile->width();
}

int ImageFileHeight(void* ptr)
{
	ImageFile* imgfile = (ImageFile*)ptr;
	return imgfile->height();
}

void* CameraListCreate()
{
	std::vector<std::string>* lst = new std::vector<std::string>;
	*lst = *Camera::s_get_list_devices();
	return lst;
}

void* CameraCreate(int idx)
{
	return new Camera(idx);
}

void CameraDestroy(void* ptr)
{
	Camera* camera = (Camera*)ptr;
	delete camera;
}

int CameraIdx(void* ptr)
{
	Camera* camera = (Camera*)ptr;
	return camera->idx();
}

int CameraWidth(void* ptr)
{
	Camera* camera = (Camera*)ptr;
	return camera->width();
}

int CameraHeight(void* ptr)
{
	Camera* camera = (Camera*)ptr;
	return camera->height();
}

void CameraAddTarget(void* ptr, void* p_target)
{
	Camera* camera = (Camera*)ptr;
	VideoTarget* target = (VideoTarget*)p_target;
	camera->AddTarget(target);
}

void* ViewerCreate(int window_width, int window_height, const char* title)
{
	return new Viewer(window_width, window_height, title);
}

void ViewerDestroy(void* ptr)
{
	Viewer* viewer = (Viewer*)ptr;
	delete viewer;
}

void ViewerSetSource(void* ptr, void* p_source)
{
	Viewer* viewer = (Viewer*)ptr;
	VideoSource* source = (VideoSource*)p_source;
	viewer->SetSource(source);
}

int ViewerDraw(void* ptr)
{
	Viewer* viewer = (Viewer*)ptr;
	return viewer->Draw()?1:0;
}

void* WindowListCreate()
{
	std::vector<std::string>* lst = new std::vector<std::string>;
	*lst = *WindowCapture::s_get_window_titles();
	return lst;
}

void* WindowCaptureCreate(int idx)
{
	return new WindowCapture(idx);
}

void WindowCaptureDestroy(void* ptr)
{
	WindowCapture* wc = (WindowCapture*)ptr;
	delete wc;
}

void* WindowCaptureGetSourcePtr(void* ptr)
{
	WindowCapture* wc = (WindowCapture*)ptr;
	return (VideoSource*)wc;
}

#include <Windows.h>
#include <mmsystem.h>

void* AudioInputDeviceListCreate()
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
	std::vector<std::string>* lst = new std::vector<std::string>;
	*lst = s_devices;
	return lst;
}

void* RecorderCreate(const char* filename, int mp4, int video_width, int video_height, int audio_device_id)
{
	return new Recorder(filename, mp4!=0, video_width, video_height, audio_device_id);
}

void RecorderDestroy(void* ptr)
{
	Recorder* recorder = (Recorder*)ptr;
	delete recorder;
}

void RecorderSetSource(void* ptr, void* p_source)
{
	Recorder* recorder = (Recorder*)ptr;
	VideoSource* source = (VideoSource*)p_source;
	recorder->SetSource(source);
}

void RecorderStart(void* ptr)
{
	Recorder* recorder = (Recorder*)ptr;
	recorder->start();
}

void RecorderStop(void* ptr)
{
	Recorder* recorder = (Recorder*)ptr;
	recorder->stop();
}
