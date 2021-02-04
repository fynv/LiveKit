import tkinter as tk
from tkinter import ttk 
import LiveKit as lk

class Recording:
    def __init__(self, lst_windows, idx_audio):
        self.window=tk.Tk()
        self.window.title("Recorder")
        self.window.geometry('350x180')

        self.label0 = tk.Label(self.window, text="Select a window:")
        self.label0.pack(padx=5, pady=5, side=tk.TOP)
       
        self.window_titles = []

        for i in range(lst_windows.size()):
            title = lst_windows.get(i)
            self.window_titles+=[title]

        self.window_selector = ttk.Combobox(self.window, width = 40) 
        self.window_selector.pack(padx=5, pady=5, side=tk.TOP)
        self.window_selector['values'] = self.window_titles
        self.window_selector.current(0)  

        self.logo_stopped = tk.PhotoImage(file="sign_red.png").subsample(3)
        self.logo_recording = tk.PhotoImage(file="sign_green.png").subsample(3)
        
        self.lbl = tk.Label(self.window, image=self.logo_stopped)
        self.lbl.pack(padx=5, pady=5, side=tk.TOP)
        self.btn = tk.Button(self.window, text="Start", command=self.start_stop)
        self.btn.pack(padx=5, pady=5, side=tk.TOP)

        self.idx_audio = idx_audio
        self.recording = False
        self.wc = None
        self.recorder = None

    def start_stop(self):
        self.recording = not self.recording
        if self.recording:            
            self.btn.configure(text = "Stop")
            self.lbl.configure(image=self.logo_recording)
            self.wc = lk.WindowCapture(self.window_selector.current())
            self.recorder = lk.Recorder("test.mp4", True, 576, 1024, self.idx_audio)
            self.recorder.set_source(self.wc)
            self.recorder.start()
        else:
            self.recorder = None
            self.wc = None
            self.btn.configure(text = "Start")
            self.lbl.configure(image=self.logo_stopped)


    def main_loop(self):
        self.window.mainloop()



lst_windows = lk.WindowList()

lst_audio_devices = lk.AudioInputDeviceList()
idx_audio = -1
for i in range(lst_audio_devices.size()):
    dev_name = lst_audio_devices.get(i)
    if dev_name.startswith("CABLE"):
        idx_audio = i
        break

rec = Recording(lst_windows, idx_audio)
rec.main_loop()

