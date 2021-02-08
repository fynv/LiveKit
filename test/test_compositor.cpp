#include <stdio.h>
#include <ImageFile.h>
#include <VideoPort.h>
#include <Camera.h>
#include <Compositor.h>
#include <Recorder.h>
using namespace LiveKit;


int main()
{
	ImageFile image_file("720.jpg");

	VideoPort port_cam;

	Camera cam;
	cam.AddTarget(&port_cam);

	Compositor scene(1280,720, 1280, 720, "Test Compositor");
	scene.SetSource(0, &image_file);
	scene.SetSource(1, &port_cam, 100, 100);

	VideoPort port_rec;
	scene.AddTarget(&port_rec);

	Recorder recorder("test.mp4", true, 1280, 720);
	recorder.SetSource(&port_rec);
	recorder.start();

	while (scene.Draw());

	return 0;
}

