import sys
import LiveKit as lk

if len(sys.argv)>1:
    lst_out = lk.AudioOutputDeviceList()
    port = lk.VideoPort()
    player = lk.Player(sys.argv[1], True, True, lst_out.id_default)
    player.add_target(port)
    width, height = player.video_size()
    width = int(width*1080/height)
    height = 1080
    scene = lk.Compositor(width, height, width, height, "Playing: "+sys.argv[1])
    scene.set_source(0, port)
    player.start()   
    while scene.draw():
        pass
