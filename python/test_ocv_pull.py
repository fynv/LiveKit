import cv2
import LiveKit as lk

port = lk.VideoPort()
cam = lk.Camera(0)
cam.add_target(port)

while(True):
    img = port.pull()[0]
    if not img is None:
        cv2.imshow('frame', img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
