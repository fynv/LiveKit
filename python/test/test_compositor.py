import LiveKit as lk

imgfile = lk.ImageFile("720.jpg")

port_cam = lk.VideoPort()
cam = lk.Camera(0)
cam.add_target(port_cam)

scene = lk.Compositor(1280,720, 1280, 720, "Test Compositor")
scene.set_source(0, imgfile)
scene.set_source(1, port_cam, (100,100))

port_rec = lk.VideoPort()
scene.add_target(port_rec)

recorder = lk.Recorder("test.mp4", True, 1280,720)
recorder.set_source(port_rec)
recorder.start()

while scene.draw():
	pass
