import os
import LiveKit as lk

lst_in = lk.AudioInputDeviceList()
print(lst_in.to_pylist())

lst_out = lk.AudioOutputDeviceList()
print(lst_out.to_pylist())

repeater = lk.AudioRepeater(lst_in.id_default, lst_out.id_default)

os.system("pause")

