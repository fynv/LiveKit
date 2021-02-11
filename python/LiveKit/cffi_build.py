import os
if os.path.exists('LiveKit/cffi.py'):
    os.remove('LiveKit/cffi.py')

import cffi
ffibuilder = cffi.FFI()
ffibuilder.set_source("LiveKit.cffi", None)

ffibuilder.cdef("""
void StrListDestroy(void* ptr);
unsigned long long StrListSize(void* ptr);
const char* StrListGet(void* ptr, int idx);

void* PullerCreate(void* p_source);
void PullerDestroy(void* ptr);
void PullerPull(void* ptr);
unsigned long long PullerTimestamp(void* ptr);
int PullerHasAlpha(void* ptr);
int PullerWidth(void* ptr);
int PullerHeight(void* ptr);
int PullerIsFlipped(void* ptr);
void PullerGetData(void* ptr, unsigned char* data);

void* PusherCreate(void* p_target);
void PusherDestroy(void* ptr);
void PusherSetSize(void* ptr, int width, int height, int has_alpha);
void PusherSetFlipped(void* ptr, int flipped);
void PusherSetData(void* ptr, const unsigned char* data);
void PusherPush(void* ptr);

void* VideoPortCreate();
void VideoPortDestroy(void* ptr);
void* VideoPortGetSourcePtr(void* ptr);
void* VideoPortGetTargetPtr(void* ptr);

void* ImageFileCreate(const char* filename);
void ImageFileDestroy(void* ptr);
void* ImageFileGetSourcePtr(void* ptr);
int ImageFileWidth(void* ptr);
int ImageFileHeight(void* ptr);

void* AudioInputDeviceListCreate();
void* AudioOutputDeviceListCreate();

void* MediaInfoCreate(const char* fn);
void MediaInfoDestroy(void* ptr);
double MediaInfoGetDuration(void* ptr);
int MediaInfoHasVideo(void* ptr);
int MediaInfoVideoWidth(void* ptr);
int MediaInfoVideoHeight(void* ptr);
double MediaInfoVideoFPS(void* ptr);
int MediaInfoVideoBitrate(void* ptr);
int MediaInfoHasAudio(void* ptr);
int MediaInfoAudioSampleRate(void* ptr);
int MediaInfoAudioNumberOfChannels(void* ptr);
int MediaInfoAudioBitrate(void* ptr);

void* PlayerCreate(const char* fn, int play_audio, int play_video, int audio_device_id);
void PlayerDestroy(void* ptr);
void PlayerAddTarget(void* ptr, void* p_target);
int PlayerVideoWidth(void* ptr);
int PlayerVideoHeight(void* ptr);    
int PlayerIsPlaying(void* ptr);
int PlayerIsEofReached(void* ptr);
double PlayerGetDuration(void* ptr);
double PlayerGetPosition(void* ptr);
void PlayerStop(void* ptr);
void PlayerStart(void* ptr);
void PlayerSetPosition(void* ptr, double pos);
void PlayerSetAudioDevice(void* ptr, int audio_device_id);

void* LazyPlayerCreate(const char* fn);
void LazyPlayerDestroy(void* ptr);
void* LazyPlayerGetSourcePtr(void* ptr);
int LazyPlayerVideoWidth(void* ptr);
int LazyPlayerVideoHeight(void* ptr);
int LazyPlayerIsPlaying(void* ptr);
int LazyPlayerIsEofReached(void* ptr);
double LazyPlayerGetDuration(void* ptr);
double LazyPlayerGetPosition(void* ptr);
void LazyPlayerStop(void* ptr);
void LazyPlayerStart(void* ptr);
void LazyPlayerSetPosition(void* ptr, double pos);

void* CameraListCreate();

void* CameraCreate(int idx);
void CameraDestroy(void* ptr);
int CameraIdx(void* ptr);
int CameraWidth(void* ptr);
int CameraHeight(void* ptr);
void CameraAddTarget(void* ptr, void* p_target);

void* ViewerCreate(int window_width, int window_height, const char* title);
void ViewerDestroy(void* ptr);
void ViewerSetSource(void* ptr, void* p_source);
int ViewerDraw(void* ptr);

void* WindowListCreate();

void* WindowCaptureCreate(int idx);
void WindowCaptureDestroy(void* ptr);
void* WindowCaptureGetSourcePtr(void* ptr);

void* RecorderCreate(const char* filename, int mp4, int video_width, int video_height, int audio_device_id);
void RecorderDestroy(void* ptr);
void RecorderSetSource(void* ptr, void* p_source);
void RecorderStart(void* ptr);
void RecorderStop(void* ptr);

void* CompositorCreate(int video_width, int video_height, int window_width, int window_height, const char* title);
void CompositorDestroy(void* ptr);
void CompositorSetVideoResolution(void* ptr, int video_width, int video_height);
int CompositorVideoWidth(void* ptr);
int CompositorVideoHeight(void* ptr);
void CompositorSetSource(void* ptr, int i, void* p_source);
void CompositorSetSource1(void* ptr, int i, void* p_source, int pos_x, int pos_y);
void CompositorSetSource2(void* ptr, int i, void* p_source, int pos_x, int pos_y, int pos_x2, int pos_y2);
void CompositorRemoveSource(void* ptr, int i);
void CompositorSetMargin(void* ptr, int margin);
int CompositorDraw(void* ptr);
void CompositorAddTarget(void* ptr, void* p_target);
""")

ffibuilder.compile()

