# LiveKit

Trying to build an interface for Python doing similar things as [OBS Studio](https://github.com/obsproject/obs-studio)
Still immature and under construction.

## Building 

LiveKit current can be built for Windows x64 platforms.

Building for Windows x86 is possible if dependecies are available.

1. Clone the repository:
```
git clone https://github.com/fynv/LiveKit.git
```

2. Download the [third-party dependency package](https://raw.githubusercontent.com/fynv/fynv.github.io/master/LiveKit-thirdparty-depends-win64.zip).

* Decompress the contents to the "thirdparty" folder.

3. Create a "build" folder and build with CMake

* Select "Visual Studio 16 2019" or "Visual Studio 15 2017".
* Select "x64"
* Click "Configure" and "Generate"
* Build with Visual Studio
* Build the "INSTALL" project

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

Unlike OBS Studio, LiveKit doesn't have a dedicated audio mixer. However, there are some components capables of audio capturing or playback. The tool [Virtual Audio Cable](https://vb-audio.com/Cable/) can be very helpful if you want to connect these components together.

## License

This source code is licensed under [GPL 2.0](https://github.com/fynv/LiveKit/raw/master/LICENSE).

