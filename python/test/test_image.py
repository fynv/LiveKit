import LiveKit as lk

imgfile = lk.ImageFile("720.jpg")
viewer = lk.Viewer(1280, 720, "Test Image")
viewer.set_source(imgfile);

while viewer.draw():
	pass



