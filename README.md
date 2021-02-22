# LiveKit

Trying to build an interface for Python doing similar things as [OBS Studio](https://github.com/obsproject/obs-studio)

## Installation

LiveKit for Windows x64 (Python 3.7) is available as a conda package:
```
conda install -c fyplus livekit
```

## Building from Source-code

LiveKit currently can be built for Windows x64 platforms.

Building for Windows x86 is possible if dependecies are available.

1. Clone the repository (including submodules):
```
git clone --recursive https://github.com/fynv/LiveKit.git
```

2. Download a FFMpeg prebuilt package

For Windows x64, a prebuilt package can be found at [https://www.gyan.dev/ffmpeg/builds/](https://www.gyan.dev/ffmpeg/builds/).

3. Create a "build" folder and build with CMake

* Select "Visual Studio 16 2019" or "Visual Studio 15 2017".
* Select "x64"
* Click "Configure"
* Set "FFMPEG_ROOT" to where you extracted FFMpeg.
* Click "Generate"
* Build with Visual Studio
* Build the "INSTALL" project

The 'VirtualCamera' sub-project can be built separately using the provided visual-studio project files. (Binaries are already provided under the VirtualCamera/bin folder)

## The Design

The system works by compositing the following kinds of objects:

* **VideoSource** objects

Video frames (images) can be retrieved from these objects.

* **VideoTarget** objects

Video frames (images) can be written to these objects.

* Objects that operates on VideoSources and VideoTargets

This includes:
    
    * Objects that read frames
    * Objects that write frames

* **VideoPort** 

A **VideoPort** object is a **VideoSource** and a **VideoTarget** at the same time. Frames written to a video-port are cached shortly and can be retrieved by other objects. This can be used to connect an object that read frames to an object that write frames.

For example, there is a class **WindowCapture**, which is a **VideoSource** class. A **WindowCapture** object can be directly connected to objects that read frames, like a **Viewer** or a **Recorder**. And there is a class **Camera**. A camera object writes frames, therefore it can be connected to objects that read frames with the help of a **VideoPort**.

You can connect these objects in different ways to accomplish different tasks.

Here is an incomplete list of what can be done at the moment.

### Viewing an image
```
ImageFile => Viewer
```

### Media-file playback
A Player object is capable of playback of a media file containing both video and audio streams. It has its own worker threads. Video frames can be written to one or more VideoTargets.

For a straight playback:

```
Player => VideoPort => Viewer
```

A LazyPlayer object playback only the video part of a media file. It works in lazy-mode, and is a VideoSource itself.

```
LazyPlayer => Viewer
```

### Viewing an camera
```
Camera => VideoPort => Viewer
```

### Recording from camera
```
Camera => VideoPort => Recorder
```

### Window capturing & recording
```
WindowCapture => Viewer
WindowCapture => Recorder
```

### Compositing
A Compositor object is one that actually works like OBS Studio. It can take in multiple VideoSources and put them at difference places to form a scene.

```
ImageFile => Compositor
Player => VideoPort => Compositor
LazyPlayer => Compositor
Camera => VideoPort =>Compositor
WindowCapture => Compositor
```

Like a Viewer, a compositor maintains its own window which shows the composited result.

Optionally, it writes to one or more VideoTargets. You can use it to record the composited video.

```
Compsitor => VideoPort => Recorder
```

Unlike OBS Studio, LiveKit doesn't have a dedicated audio mixer. However, there are some components capables of audio capturing or playback. The tool [Virtual Audio Cable](https://vb-audio.com/Cable/) can be very helpful if you want to connect these components together.

### IPC Video Transmission

Interprocess video transmission can be done by a pair of IPCTarget and IPCSource objects, which uses named file-mapping to efficiently share the frames.

The 'VirtualCamera' sub-project provides a dshow virtual-camera which is compatible with IPCTarget. 

Registering the virtual camera:
```
VirtualCamera\bin> register.cmd
```

Mapping-name and virtual resolution must be set before using the virtual camera from the host process, whcih can be done by setting a couple of registry values. See VirtualCamera/bin/reg_config.py for details.

A IPCTarget object can be used to feed content into the virtual camera. 

```python
import LiveKit as lk
target = lk.IPCTarget("LiveKitVCam", width, height, False)
```

Here the mapping-name and resolution information of the IPCTarget object must match the virtual-camera settings.


## License

This source code is provided to you by Vulcan Eon (北京鲜衣怒马文化传媒有限公司) 
and is licensed under [GPL 2.0](https://github.com/fynv/LiveKit/blob/master/LICENSE).

