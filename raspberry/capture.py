#!/usr/bin/python3
import time
import picamera
import os.path 
import io
import datetime
from StreamPump import StreamPump

with picamera.PiCamera() as camera:
    with io.open("myvid.yuv", mode="wb") as file:
        destination = StreamPump(file, 1024*1024*300)
        destination.start()
        camera.resolution = (1280, 720)
        camera.framerate = 13
        camera.vflip = True
        camera.hflip = True
        camera.start_preview()
        time.sleep(2)
        camera.start_recording(destination, format='yuv')
        camera.wait_recording(120)
        camera.stop_recording()
        sFlushTime = datetime.datetime.now().strftime("%H:%M:%S")

        print("Flush now" + sFlushTime)
        destination.finish()
        print("Flush done" + datetime.datetime.now().strftime("%H:%M:%S"))
