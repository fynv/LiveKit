#include <stdio.h>
#include <WindowCapture.h>
#include <Viewer.h>
using namespace LiveKit;

#include <string>

int main()
{
	int idx = -1;
	const std::vector<std::string>* lst = WindowCapture::s_get_window_titles();
	for (size_t i = 0; i < lst->size(); i++)
	{
		if ((*lst)[i].find("Ò¹Éñ") != std::string::npos)
		{
			idx = (int)i;
			break;
		}
	}
	if (idx>=0)
	{
		WindowCapture wc(idx);
		Viewer viewer(480, 854, "Test Window Capture");
		viewer.SetSource(&wc);
		while (viewer.Draw());
	}

	return 0;
}

