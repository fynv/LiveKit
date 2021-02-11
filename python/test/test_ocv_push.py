import cv2
import LiveKit as lk

img = cv2.imread('720.jpg')
port = lk.VideoPort()
port.push(img)

viewer = lk.Viewer(1280, 720, "Test OpenCV Push")
viewer.set_source(port);
while viewer.draw():
    pass



