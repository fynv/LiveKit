import LiveKit as lk

lst = lk.WindowList()
idx = -1
for i in range(lst.size()):
	title = lst.get(i)
	if "夜神" in title:
		idx = i
		break
if idx >=0:
	wc = lk.WindowCapture(idx)
	viewer = lk.Viewer(480, 854, "Test Window Capture") 
	viewer.set_source(wc)
	while viewer.draw():
		pass
