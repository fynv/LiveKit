import os

from cffi import FFI
ffi  = FFI()

ffi.cdef("""    
void StrListDestroy(void* ptr);
unsigned long long StrListSize(void* ptr);
const char* StrListGet(void* ptr, int idx);

void* VideoPortCreate();
void VideoPortDestroy(void* ptr);
void* VideoPortGetSourcePtr(void* ptr);
void* VideoPortGetTargetPtr(void* ptr);

void* ImageFileCreate(const char* filename);
void ImageFileDestroy(void* ptr);
void* ImageFileGetSourcePtr(void* ptr);
int ImageFileWidth(void* ptr);
int ImageFileHeight(void* ptr);

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

void* AudioInputDeviceListCreate();

void* RecorderCreate(const char* filename, int mp4, int video_width, int video_height, int audio_device_id);
void RecorderDestroy(void* ptr);
void RecorderSetSource(void* ptr, void* p_source);
void RecorderStart(void* ptr);
void RecorderStop(void* ptr);
""")

if os.name == 'nt':
    Native = ffi.dlopen('PyLiveKit.dll')
elif os.name == "posix":
    Native = ffi.dlopen('libPyLiveKit.so')

class StringList:
    def __del__(self):
        Native.StrListDestroy(self.cptr)

    def size(self):
        return Native.StrListSize(self.cptr)

    def get(self, i):
        return ffi.string(Native.StrListGet(self.cptr, i)).decode('mbcs')

    def to_pylist(self):
        return [self.get(i) for i in range(self.size())]

class VideoPort: # video-source & video-target
    def __init__(self):
        self.cptr = Native.VideoPortCreate()
        self.source_ptr = Native.VideoPortGetSourcePtr(self.cptr)
        self.target_ptr = Native.VideoPortGetTargetPtr(self.cptr)

    def __del__(self):
        Native.VideoPortDestroy(self.cptr)

class ImageFile: # video-source
    def __init__(self, filename):
        self.cptr = Native.ImageFileCreate(filename.encode('mbcs'))
        self.source_ptr = Native.ImageFileGetSourcePtr(self.cptr)

    def __del__(self):
        Native.ImageFileDestroy(self.cptr)

    def size(self):
        return Native.ImageFileWidth(self.cptr), Native.ImageFileHeight(self.cptr)

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

    def add_target(self, target): # slot for video-targets
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

class WindowCapture: # video-source
    def __init__(self, idx):
        self.cptr = Native.WindowCaptureCreate(idx)
        self.source_ptr = Native.WindowCaptureGetSourcePtr(self.cptr)

    def __del__(self):
        Native.WindowCaptureDestroy(self.cptr)

class AudioInputDeviceList(StringList):
    def __init__(self):
        self.cptr = Native.AudioInputDeviceListCreate()

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
 