#include <stdio.h>
#include <VideoPort.h>
#include <Camera.h>
#include <Viewer.h>
using namespace LiveKit;

int main()
{	
	VideoPort port;

	Camera cam;
	cam.AddTarget(&port);

	Viewer viewer(640, 480, "Test Camera");
	viewer.SetSource(&port);

	while (viewer.Draw());

	return 0;
}

