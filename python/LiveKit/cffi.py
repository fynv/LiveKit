# auto-generated file
import _cffi_backend

ffi = _cffi_backend.FFI('LiveKit.cffi',
    _version = 0x2601,
    _types = b'\x00\x00\x0E\x0D\x00\x00\x6B\x03\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x36\x0D\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x02\x0D\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x6A\x0D\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x68\x03\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x0E\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x0E\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x0E\x11\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x0E\x11\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x01\x0D\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x0E\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x01\x11\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x07\x01\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x69\x03\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x69\x03\x00\x00\x00\x0F\x00\x00\x6B\x0D\x00\x00\x01\x11\x00\x00\x01\x11\x00\x00\x00\x0F\x00\x00\x02\x01\x00\x00\x04\x01\x00\x00\x0C\x01\x00\x00\x00\x01',
    _globals = (b'\x00\x00\x2F\x23AudioInputDeviceListCreate',0,b'\x00\x00\x2F\x23AudioOutputDeviceListCreate',0,b'\x00\x00\x64\x23CameraAddTarget',0,b'\x00\x00\x1D\x23CameraCreate',0,b'\x00\x00\x31\x23CameraDestroy',0,b'\x00\x00\x07\x23CameraHeight',0,b'\x00\x00\x07\x23CameraIdx',0,b'\x00\x00\x2F\x23CameraListCreate',0,b'\x00\x00\x07\x23CameraWidth',0,b'\x00\x00\x64\x23CompositorAddTarget',0,b'\x00\x00\x25\x23CompositorCreate',0,b'\x00\x00\x31\x23CompositorDestroy',0,b'\x00\x00\x07\x23CompositorDraw',0,b'\x00\x00\x38\x23CompositorRemoveSource',0,b'\x00\x00\x38\x23CompositorSetMargin',0,b'\x00\x00\x47\x23CompositorSetSource',0,b'\x00\x00\x4C\x23CompositorSetSource1',0,b'\x00\x00\x53\x23CompositorSetSource2',0,b'\x00\x00\x3C\x23CompositorSetVideoResolution',0,b'\x00\x00\x07\x23CompositorVideoHeight',0,b'\x00\x00\x07\x23CompositorVideoWidth',0,b'\x00\x00\x0D\x23ImageFileCreate',0,b'\x00\x00\x31\x23ImageFileDestroy',0,b'\x00\x00\x2C\x23ImageFileGetSourcePtr',0,b'\x00\x00\x07\x23ImageFileHeight',0,b'\x00\x00\x07\x23ImageFileWidth',0,b'\x00\x00\x0D\x23LazyPlayerCreate',0,b'\x00\x00\x31\x23LazyPlayerDestroy',0,b'\x00\x00\x04\x23LazyPlayerGetDuration',0,b'\x00\x00\x04\x23LazyPlayerGetPosition',0,b'\x00\x00\x2C\x23LazyPlayerGetSourcePtr',0,b'\x00\x00\x07\x23LazyPlayerIsEofReached',0,b'\x00\x00\x07\x23LazyPlayerIsPlaying',0,b'\x00\x00\x34\x23LazyPlayerSetPosition',0,b'\x00\x00\x31\x23LazyPlayerStart',0,b'\x00\x00\x31\x23LazyPlayerStop',0,b'\x00\x00\x07\x23LazyPlayerVideoHeight',0,b'\x00\x00\x07\x23LazyPlayerVideoWidth',0,b'\x00\x00\x07\x23MediaInfoAudioBitrate',0,b'\x00\x00\x07\x23MediaInfoAudioNumberOfChannels',0,b'\x00\x00\x07\x23MediaInfoAudioSampleRate',0,b'\x00\x00\x0D\x23MediaInfoCreate',0,b'\x00\x00\x31\x23MediaInfoDestroy',0,b'\x00\x00\x04\x23MediaInfoGetDuration',0,b'\x00\x00\x07\x23MediaInfoHasAudio',0,b'\x00\x00\x07\x23MediaInfoHasVideo',0,b'\x00\x00\x07\x23MediaInfoVideoBitrate',0,b'\x00\x00\x04\x23MediaInfoVideoFPS',0,b'\x00\x00\x07\x23MediaInfoVideoHeight',0,b'\x00\x00\x07\x23MediaInfoVideoWidth',0,b'\x00\x00\x64\x23PlayerAddTarget',0,b'\x00\x00\x10\x23PlayerCreate',0,b'\x00\x00\x31\x23PlayerDestroy',0,b'\x00\x00\x04\x23PlayerGetDuration',0,b'\x00\x00\x04\x23PlayerGetPosition',0,b'\x00\x00\x07\x23PlayerIsEofReached',0,b'\x00\x00\x07\x23PlayerIsPlaying',0,b'\x00\x00\x38\x23PlayerSetAudioDevice',0,b'\x00\x00\x34\x23PlayerSetPosition',0,b'\x00\x00\x31\x23PlayerStart',0,b'\x00\x00\x31\x23PlayerStop',0,b'\x00\x00\x07\x23PlayerVideoHeight',0,b'\x00\x00\x07\x23PlayerVideoWidth',0,b'\x00\x00\x2C\x23PullerCreate',0,b'\x00\x00\x31\x23PullerDestroy',0,b'\x00\x00\x5C\x23PullerGetData',0,b'\x00\x00\x07\x23PullerHasAlpha',0,b'\x00\x00\x07\x23PullerHeight',0,b'\x00\x00\x07\x23PullerIsFlipped',0,b'\x00\x00\x31\x23PullerPull',0,b'\x00\x00\x0A\x23PullerTimestamp',0,b'\x00\x00\x07\x23PullerWidth',0,b'\x00\x00\x2C\x23PusherCreate',0,b'\x00\x00\x31\x23PusherDestroy',0,b'\x00\x00\x31\x23PusherPush',0,b'\x00\x00\x60\x23PusherSetData',0,b'\x00\x00\x38\x23PusherSetFlipped',0,b'\x00\x00\x41\x23PusherSetSize',0,b'\x00\x00\x16\x23RecorderCreate',0,b'\x00\x00\x31\x23RecorderDestroy',0,b'\x00\x00\x64\x23RecorderSetSource',0,b'\x00\x00\x31\x23RecorderStart',0,b'\x00\x00\x31\x23RecorderStop',0,b'\x00\x00\x31\x23StrListDestroy',0,b'\x00\x00\x00\x23StrListGet',0,b'\x00\x00\x0A\x23StrListSize',0,b'\x00\x00\x2F\x23VideoPortCreate',0,b'\x00\x00\x31\x23VideoPortDestroy',0,b'\x00\x00\x2C\x23VideoPortGetSourcePtr',0,b'\x00\x00\x2C\x23VideoPortGetTargetPtr',0,b'\x00\x00\x20\x23ViewerCreate',0,b'\x00\x00\x31\x23ViewerDestroy',0,b'\x00\x00\x07\x23ViewerDraw',0,b'\x00\x00\x64\x23ViewerSetSource',0,b'\x00\x00\x1D\x23WindowCaptureCreate',0,b'\x00\x00\x31\x23WindowCaptureDestroy',0,b'\x00\x00\x2C\x23WindowCaptureGetSourcePtr',0,b'\x00\x00\x2F\x23WindowListCreate',0),
)