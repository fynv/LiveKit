import os

from cffi import FFI
ffi  = FFI()

ffi.cdef("""    
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

if os.name == 'nt':
    Native = ffi.dlopen('PyLiveKit.dll')
elif os.name == "posix":
    Native = ffi.dlopen('libPyLiveKit.so')

import numpy as np

class StringList:
    def __del__(self):
        Native.StrListDestroy(self.cptr)

    def size(self):
        return Native.StrListSize(self.cptr)

    def get(self, i):
        return ffi.string(Native.StrListGet(self.cptr, i)).decode('mbcs')

    def to_pylist(self):
        return [self.get(i) for i in range(self.size())]

class VideoSource:
    def __init__(self):
        self.puller_ptr = Native.PullerCreate(self.source_ptr)

    def __del__(self):
        Native.PullerDestroy(self.puller_ptr)

    def pull(self):
        Native.PullerPull(self.puller_ptr)
        timestamp = Native.PullerTimestamp(self.puller_ptr)
        if timestamp == 0xFFFFFFFFFFFFFFFF:
            return None, timestamp, False
        chn = 3
        if Native.PullerHasAlpha(self.puller_ptr)!=0:
            chn = 4
        width = Native.PullerWidth(self.puller_ptr)
        height = Native.PullerHeight(self.puller_ptr)
        is_flipped = Native.PullerIsFlipped(self.puller_ptr)        
        arr = np.empty((height, width, chn), dtype=np.uint8)
        Native.PullerGetData(self.puller_ptr, ffi.cast("unsigned char *", arr.__array_interface__['data'][0]))
        return arr, timestamp, is_flipped

class VideoTarget:
    def __init__(self):
        self.pusher_ptr = Native.PusherCreate(self.target_ptr)

    def __del__(self):
        Native.PusherDestroy(self.pusher_ptr)

    def push(self, arr, is_flipped=False):
        width = arr.shape[1]
        height = arr.shape[0]
        has_alpha = arr.shape[2]>3
        Native.PusherSetSize(self.pusher_ptr, width, height, has_alpha)
        Native.PusherSetFlipped(self.pusher_ptr, is_flipped)
        Native.PusherSetData(self.pusher_ptr, ffi.cast("const unsigned char *", arr.__array_interface__['data'][0]))
        Native.PusherPush(self.pusher_ptr)


class VideoPort(VideoSource, VideoTarget):
    def __init__(self):
        self.cptr = Native.VideoPortCreate()
        self.source_ptr = Native.VideoPortGetSourcePtr(self.cptr)
        self.target_ptr = Native.VideoPortGetTargetPtr(self.cptr)
        VideoSource.__init__(self)
        VideoTarget.__init__(self)

    def __del__(self):
        VideoTarget.__del__(self)
        VideoSource.__del__(self)
        Native.VideoPortDestroy(self.cptr)

class ImageFile(VideoSource):
    def __init__(self, filename):
        self.cptr = Native.ImageFileCreate(filename.encode('mbcs'))
        self.source_ptr = Native.ImageFileGetSourcePtr(self.cptr)
        VideoSource.__init__(self)

    def __del__(self):
        VideoSource.__del__(self)
        Native.ImageFileDestroy(self.cptr)

    def size(self):
        return Native.ImageFileWidth(self.cptr), Native.ImageFileHeight(self.cptr)

class AudioInputDeviceList(StringList):
    def __init__(self):
        self.cptr = Native.AudioInputDeviceListCreate()

class AudioOutputDeviceList(StringList):
    def __init__(self):
        self.cptr = Native.AudioOutputDeviceListCreate() 

class MediaInfo:
    def __init__(self, filename):
        self.cptr = Native.MediaInfoCreate(filename.encode('mbcs'))

    def __del__(self):
        Native.MediaInfoDestroy(self.cptr)

    def duration(self):
        return Native.MediaInfoGetDuration(self.cptr)

    def has_video(self):
        return Native.MediaInfoHasVideo(self.cptr)!=0

    def video_size(self):
        return (Native.MediaInfoVideoWidth(self.cptr), Native.MediaInfoVideoHeight(self.cptr))

    def video_fps(self):
        return Native.MediaInfoVideoFPS(self.cptr)

    def video_bitrate(self):
        return Native.MediaInfoVideoBitrate(self.cptr)

    def has_audio(self):
        return Native.MediaInfoHasAudio(self.cptr)!=0

    def audio_sample_rate(self):
        return Native.MediaInfoAudioSampleRate(self.cptr)

    def audio_number_of_channels(self):
        return Native.MediaInfoAudioNumberOfChannels(self.cptr)

    def audio_bitrate(self):
        return Native.MediaInfoAudioBitrate(self.cptr)

class Player:
    def __init__(self, filename, play_audio = True, play_video = True, audio_device_id = 0):
        self.cptr = Native.PlayerCreate(filename.encode('mbcs'), play_audio, play_video, audio_device_id)
        self.targets = []

    def __del__(self):
        Native.PlayerDestroy(self.cptr)

    def add_target(self, target): # slots for video-targets
        self.targets += [target]
        Native.PlayerAddTarget(self.cptr, target.target_ptr)

    def video_size(self):
        return Native.PlayerVideoWidth(self.cptr), Native.PlayerVideoHeight(self.cptr)

    def is_playing(self):
        return Native.PlayerIsPlaying(self.cptr)!=0

    def is_eof_reached(self):
        return Native.PlayerIsEofReached(self.cptr)!=0

    def get_duration(self):
        return Native.PlayerGetDuration(self.cptr)

    def get_position(self):
        return Native.PlayerGetPosition(self.cptr)

    def stop(self):
        Native.PlayerStop(self.cptr)

    def start(self):
        Native.PlayerStart(self.cptr)

    def set_position(self, pos):
        Native.PlayerSetPosition(self.cptr, pos)

    def set_audio_device(self, audio_device_id):
        Native.PlayerSetAudioDevice(self.cptr, audio_device_id)

class LazyPlayer(VideoSource):
    def __init__(self, filename):
        self.cptr = Native.LazyPlayerCreate(filename.encode('mbcs'))
        self.source_ptr = Native.LazyPlayerGetSourcePtr(self.cptr)
        VideoSource.__init__(self)

    def __del__(self):
        VideoSource.__del__(self)
        Native.LazyPlayerDestroy(self.cptr)

    def video_size(self):
        return Native.LazyPlayerVideoWidth(self.cptr), Native.LazyPlayerVideoHeight(self.cptr)

    def is_playing(self):
        return Native.LazyPlayerIsPlaying(self.cptr)!=0

    def is_eof_reached(self):
        return Native.LazyPlayerIsEofReached(self.cptr)!=0

    def get_duration(self):
        return Native.LazyPlayerGetDuration(self.cptr)

    def get_position(self):
        return Native.LazyPlayerGetPosition(self.cptr)

    def stop(self):
        Native.LazyPlayerStop(self.cptr)

    def start(self):
        Native.LazyPlayerStart(self.cptr)

    def set_position(self, pos):
        Native.LazyPlayerSetPosition(self.cptr, pos)


class CameraList(StringList):
    def __init__(self):
        self.cptr = Native.CameraListCreate()

class Camera:
    def __init__(self, idx = 0):
        self.cptr = Native.CameraCreate(idx)
        self.targets = []

    def __del__(self):
        Native.CameraDestroy(self.cptr)

    def size(self):
        return Native.CameraWidth(self.cptr), Native.CameraHeight(self.cptr)

    def add_target(self, target): # slots for video-targets
        self.targets += [target]
        Native.CameraAddTarget(self.cptr, target.target_ptr)

class Viewer:
    def __init__(self, window_width, window_height, title):
        self.cptr = Native.ViewerCreate(window_width, window_height, title.encode('utf-8'))

    def __del__(self):
        Native.ViewerDestroy(self.cptr)

    def set_source(self, source): # slot for a video-source
        self.source = source
        Native.ViewerSetSource(self.cptr, source.source_ptr)

    def draw(self):
        return Native.ViewerDraw(self.cptr)!=0

class WindowList(StringList):
    def __init__(self):
        self.cptr = Native.WindowListCreate()

class WindowCapture(VideoSource):
    def __init__(self, idx):
        self.cptr = Native.WindowCaptureCreate(idx)
        self.source_ptr = Native.WindowCaptureGetSourcePtr(self.cptr)
        VideoSource.__init__(self)

    def __del__(self):
        VideoSource.__del__(self)
        Native.WindowCaptureDestroy(self.cptr)

class Recorder:
    def __init__(self, filename, mp4, video_width, video_height, audio_device_id = -1):
        self.cptr = Native.RecorderCreate(filename.encode('mbcs'), mp4, video_width, video_height, audio_device_id)

    def __del__(self):
        Native.RecorderDestroy(self.cptr)

    def set_source(self, source):  # slot for a video-source
        self.source = source
        Native.RecorderSetSource(self.cptr, source.source_ptr)

    def start(self):
        Native.RecorderStart(self.cptr)

    def stop(self):
        Native.RecorderStop(self.cptr)
 
class Compositor:
    def __init__(self, video_width, video_height, window_width, window_height, title):
        self.cptr = Native.CompositorCreate(video_width, video_height, window_width, window_height, title.encode('utf-8'))
        self.sources = []
        self.targets = []

    def __del__(self):
        Native.CompositorDestroy(self.cptr)

    def set_video_resolution(self, video_width, video_height):
        Native.CompositorSetVideoResolution(self.cptr, video_width, video_height)

    def video_size(self):
        return Native.CompositorVideoWidth(self.cptr), Native.CompositorVideoHeight(self.cptr)

    def set_source(self, idx, source, upper_left=None, lower_right= None): # slots for a video-source
        while idx>=len(self.sources):
            self.sources.append(None)

        if source is None:
            Native.CompositorRemoveSource(self.cptr, idx)
        else:
            mode = 0 # stretch-full
            if not upper_left is None:
                mode = 1 # move 
                if not lower_right is None:
                    mode = 2 # move & stretch        
            if mode == 0:
                Native.CompositorSetSource(self.cptr, idx, source.source_ptr)
            elif mode == 1:
                Native.CompositorSetSource1(self.cptr, idx, source.source_ptr, upper_left[0], upper_left[1])
            else:
                Native.CompositorSetSource2(self.cptr, idx, source.source_ptr, upper_left[0], upper_left[1], lower_right[0], lower_right[1])   

        self.sources[idx] = source

    def remove_source(self, idx):
        self.set_source(idx, None)

    def set_margin(self, margin):
        Native.CompositorSetMargin(self.cptr, margin)

    def draw(self):
        return Native.CompositorDraw(self.cptr)!=0

    def add_target(self, target): # slots for video-targets
        self.targets += [target]
        Native.CompositorAddTarget(self.cptr, target.target_ptr)

