import os
import LiveKit as lk

cam = lk.Camera(0)
width, height = cam.size()
target = lk.IPCTarget("LiveKitIPC", width, height, False)
cam.add_target(target)
os.system("pause")
