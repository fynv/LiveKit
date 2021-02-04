#include <stdio.h>
#include <ImageFile.h>
#include <Viewer.h>
using namespace LiveKit;


int main()
{
	ImageFile image_file("720.jpg");
	Viewer viewer(1280, 720, "Test Image");
	viewer.SetSource(&image_file);	
	while (viewer.Draw());
	
	return 0;
}

