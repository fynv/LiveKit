import tkinter as tk
from tkinter import ttk 
import LiveKit as lk

class WindowSelect:
    def __init__(self, lst_windows):    
        self.window=tk.Tk()
        self.window.title("Window Select")

        self.label0 = tk.Label(self.window, text="Select a window:")
        self.label0.pack(padx=5, pady=5, side=tk.TOP)

        self.window_titles = lst_windows

        self.window_selector = ttk.Combobox(self.window, width = 40) 
        self.window_selector.pack(padx=5, pady=5, side=tk.TOP)
        self.window_selector['values'] = self.window_titles
        self.window_selector.current(0)

        self.btn = tk.Button(self.window, text="Ok", command=self.Ok)
        self.btn.pack(padx=5, pady=5, side=tk.TOP)

        self.idx_wnd = -1

    def Ok(self):
        self.idx_wnd = self.window_selector.current()  
        self.window.destroy()

    def main_loop(self):
        self.window.mainloop()

lst = lk.WindowList().to_pylist()
wsel = WindowSelect(lst)
wsel.main_loop()

if wsel.idx_wnd>=0:
    wc = lk.WindowCapture(wsel.idx_wnd)
    viewer = lk.Viewer(480, 854, "Test Window Capture") 
    viewer.set_source(wc)
    while viewer.draw():
        pass
