import LiveKit as lk

source = lk.IPCSource("LiveKitIPC")
viewer = lk.Viewer(640, 480, "Test IPC")
viewer.set_source(source);
while viewer.draw():
	pass



