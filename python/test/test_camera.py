import LiveKit as lk

lst = lk.CameraList()
for i in range(lst.size()):
	print(lst.get(i))

port = lk.VideoPort()
cam = lk.Camera(0)
cam.add_target(port)
viewer = lk.Viewer(640, 480, "Test Camera")
viewer.set_source(port);
while viewer.draw():
	pass


