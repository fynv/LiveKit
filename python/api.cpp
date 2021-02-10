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

	PY_LiveKit_API void* AudioInputDeviceListCreate();
	PY_LiveKit_API void* AudioOutputDeviceListCreate();

	PY_LiveKit_API void* MediaInfoCreate(const char* fn);
	PY_LiveKit_API void MediaInfoDestroy(void* ptr);
	PY_LiveKit_API double MediaInfoGetDuration(void* ptr);
	PY_LiveKit_API int MediaInfoHasVideo(void* ptr);
	PY_LiveKit_API int MediaInfoVideoWidth(void* ptr);
	PY_LiveKit_API int MediaInfoVideoHeight(void* ptr);
	PY_LiveKit_API double MediaInfoVideoFPS(void* ptr);
	PY_LiveKit_API int MediaInfoVideoBitrate(void* ptr);
	PY_LiveKit_API int MediaInfoHasAudio(void* ptr);
	PY_LiveKit_API int MediaInfoAudioSampleRate(void* ptr);
	PY_LiveKit_API int MediaInfoAudioNumberOfChannels(void* ptr);
	PY_LiveKit_API int MediaInfoAudioBitrate(void* ptr);

	PY_LiveKit_API void* PlayerCreate(const char* fn, int play_audio, int play_video, int audio_device_id);
	PY_LiveKit_API void PlayerDestroy(void* ptr);
	PY_LiveKit_API void PlayerAddTarget(void* ptr, void* p_target);
	PY_LiveKit_API double PlayerGetDuration(void* ptr);
	PY_LiveKit_API int PlayerIsPlaying(void* ptr);
	PY_LiveKit_API int PlayerIsEofReached(void* ptr);
	PY_LiveKit_API double PlayerGetPosition(void* ptr);
	PY_LiveKit_API int PlayerVideoWidth(void* ptr);
	PY_LiveKit_API int PlayerVideoHeight(void* ptr);
	PY_LiveKit_API void PlayerStop(void* ptr);
	PY_LiveKit_API void PlayerStart(void* ptr);
	PY_LiveKit_API void PlayerSetPosition(void* ptr, double pos);
	PY_LiveKit_API void PlayerSetAudioDevice(void* ptr, int audio_device_id);

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

	PY_LiveKit_API void* RecorderCreate(const char* filename, int mp4, int video_width, int video_height, int audio_device_id);
	PY_LiveKit_API void RecorderDestroy(void* ptr);
	PY_LiveKit_API void RecorderSetSource(void* ptr, void* p_source);
	PY_LiveKit_API void RecorderStart(void* ptr);
	PY_LiveKit_API void RecorderStop(void* ptr);

	PY_LiveKit_API void* CompositorCreate(int video_width, int video_height, int window_width, int window_height, const char* title);
	PY_LiveKit_API void CompositorDestroy(void* ptr);
	PY_LiveKit_API void CompositorSetVideoResolution(void* ptr, int video_width, int video_height);
	PY_LiveKit_API int CompositorVideoWidth(void* ptr);
	PY_LiveKit_API int CompositorVideoHeight(void* ptr);
	PY_LiveKit_API void CompositorSetSource(void* ptr, int i, void* p_source);
	PY_LiveKit_API void CompositorSetSource1(void* ptr, int i, void* p_source, int pos_x, int pos_y);
	PY_LiveKit_API void CompositorSetSource2(void* ptr, int i, void* p_source, int pos_x, int pos_y, int pos_x2, int pos_y2);
	PY_LiveKit_API void CompositorRemoveSource(void* ptr, int i);
	PY_LiveKit_API void CompositorSetMargin(void* ptr, int margin);
	PY_LiveKit_API int CompositorDraw(void* ptr);
	PY_LiveKit_API void CompositorAddTarget(void* ptr, void* p_target);

}

#include <VideoPort.h>
#include <ImageFile.h>
#include <Player.h>
#include <Camera.h>
#include <Viewer.h>
#include <WindowCapture.h>
#include <Recorder.h>
#include <Compositor.h>
using namespace LiveKit;

#include <vector>
#include <string>

#include <Windows.h>
#include <mmsystem.h>

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

void* AudioOutputDeviceListCreate()
{
	static std::vector<std::string> s_devices;
	if (s_devices.size() == 0)
	{
		unsigned num_dev = waveOutGetNumDevs();
		WAVEOUTCAPS waveOutDevCaps;
		for (unsigned i = 0; i < num_dev; i++)
		{
			waveOutGetDevCaps(i, &waveOutDevCaps, sizeof(WAVEOUTCAPS));
			s_devices.push_back(waveOutDevCaps.szPname);
		}
	}
	std::vector<std::string>* lst = new std::vector<std::string>;
	*lst = s_devices;
	return lst;
}

void* MediaInfoCreate(const char* fn)
{
	MediaInfo* info = new MediaInfo;
	get_media_info(fn, info);
	return info;
}

void MediaInfoDestroy(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	delete info;
}


double MediaInfoGetDuration(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return (double)info->duration/(1000000.0);
}

int MediaInfoHasVideo(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->has_video ? 1 : 0;
}


int MediaInfoVideoWidth(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->video_width;
}

int MediaInfoVideoHeight(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->video_height;
}

double MediaInfoVideoFPS(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->video_fps;
}

int MediaInfoVideoBitrate(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->video_bitrate;
}

int MediaInfoHasAudio(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->has_audio ? 1 : 0;
}

int MediaInfoAudioSampleRate(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->audio_sample_rate;
}

int MediaInfoAudioNumberOfChannels(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->audio_number_of_channels;
}

int MediaInfoAudioBitrate(void* ptr)
{
	MediaInfo* info = (MediaInfo*)ptr;
	return info->audio_bitrate;
}

void* PlayerCreate(const char* fn, int play_audio, int play_video, int audio_device_id)
{
	return new Player(fn, play_audio != 0, play_video != 0, audio_device_id);
}

void PlayerDestroy(void* ptr)
{
	Player* player = (Player*)ptr;
	delete player;
}

void PlayerAddTarget(void* ptr, void* p_target)
{
	Player* player = (Player*)ptr;
	VideoTarget* target = (VideoTarget*)p_target;
	player->AddTarget(target);
}

double PlayerGetDuration(void* ptr)
{
	Player* player = (Player*)ptr;
	return (double)player->get_duration() / 1000000.0;
}

int PlayerIsPlaying(void* ptr)
{
	Player* player = (Player*)ptr;
	return player->is_playing() ? 1 : 0;
}

int PlayerIsEofReached(void* ptr)
{
	Player* player = (Player*)ptr;
	return player->is_eof_reached() ? 1 : 0;
}

double PlayerGetPosition(void* ptr)
{
	Player* player = (Player*)ptr;
	return (double)player->get_position() / 1000000.0;
}

int PlayerVideoWidth(void* ptr)
{
	Player* player = (Player*)ptr;
	return player->video_width();
}

int PlayerVideoHeight(void* ptr)
{
	Player* player = (Player*)ptr;
	return player->video_height();
}

void PlayerStop(void* ptr)
{
	Player* player = (Player*)ptr;
	player->stop();
}

void PlayerStart(void* ptr)
{
	Player* player = (Player*)ptr;
	player->start();
}

void PlayerSetPosition(void* ptr, double pos)
{
	Player* player = (Player*)ptr;
	player->set_position((uint64_t)(pos*1000000.0));
}

void PlayerSetAudioDevice(void* ptr, int audio_device_id)
{
	Player* player = (Player*)ptr;
	player->set_audio_device(audio_device_id);
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

void* CompositorCreate(int video_width, int video_height, int window_width, int window_height, const char* title)
{
	return new Compositor(video_width, video_height, window_width, window_height, title);
}

void CompositorDestroy(void* ptr)
{
	Compositor* comp = (Compositor*)ptr;
	delete comp;
}

void CompositorSetVideoResolution(void* ptr, int video_width, int video_height)
{
	Compositor* comp = (Compositor*)ptr;
	comp->SetVideoResolution(video_width, video_height);
}

int CompositorVideoWidth(void* ptr)
{
	Compositor* comp = (Compositor*)ptr;
	return comp->VideoWidth();
}

int CompositorVideoHeight(void* ptr)
{
	Compositor* comp = (Compositor*)ptr;
	return comp->VideoHeight();
}

void CompositorSetSource(void* ptr, int i, void* p_source)
{
	Compositor* comp = (Compositor*)ptr;
	VideoSource* source = (VideoSource*)p_source;
	comp->SetSource(i, source);
}

void CompositorSetSource1(void* ptr, int i, void* p_source, int pos_x, int pos_y)
{
	Compositor* comp = (Compositor*)ptr;
	VideoSource* source = (VideoSource*)p_source;
	comp->SetSource(i, source, pos_x, pos_y);
}

void CompositorSetSource2(void* ptr, int i, void* p_source, int pos_x, int pos_y, int pos_x2, int pos_y2)
{
	Compositor* comp = (Compositor*)ptr;
	VideoSource* source = (VideoSource*)p_source;
	comp->SetSource(i, source, pos_x, pos_y, pos_x2, pos_y2);
}

void CompositorRemoveSource(void* ptr, int i)
{
	Compositor* comp = (Compositor*)ptr;
	comp->RemoveSource(i);
}

void CompositorSetMargin(void* ptr, int margin)
{
	Compositor* comp = (Compositor*)ptr;
	comp->SetMargin(margin);
}

int CompositorDraw(void* ptr)
{
	Compositor* comp = (Compositor*)ptr;
	return comp->Draw() ? 1 : 0;
}

void CompositorAddTarget(void* ptr, void* p_target)
{
	Compositor* comp = (Compositor*)ptr;
	VideoTarget* target = (VideoTarget*)p_target;
	comp->AddTarget(target);
}
