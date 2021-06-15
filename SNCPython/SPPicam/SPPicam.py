'''
////////////////////////////////////////////////////////////////////////////
//
//  This file is part of SNC
//
//  Copyright (c) 2014-2021, Richard Barnett
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of 
//  this software and associated documentation files (the "Software"), to deal in 
//  the Software without restriction, including without limitation the rights to use, 
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the 
//  Software, and to permit persons to whom the Software is furnished to do so, 
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all 
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
'''

# camera parameters - change as required

CAMERA_INDEX = 0
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 480
CAMERA_RATE = 10

# Now onto the real stuff

import SyntroPython
import SyntroPythonPy
import sys
import time
import io
import picamera

# this is used to track when we have a new frame
lastFrameIndex = -1

def sendFrame(stream):
    ''' sendFrame checks the circular buffer to see if there is a new frame
    and send it on the SyntroLink if that's possible '''

    global lastFrameIndex

    with stream.lock:
        # skip to last frame in iteration
        frame = None
        for frame in stream.frames:
            pass

        if (frame is None):
            return
        # check to see if this is new
        if (frame.index == lastFrameIndex):
            return

        # record index of new frame
        lastFrameIndex = frame.index

        # get the frame data and display it
        stream.seek(frame.position)
        image = stream.read(frame.frame_size)
        SyntroPython.displayJpegImage(image, "")

        # now check if it can be sent on the SyntroLink
        if (SyntroPython.isServiceActive(servicePort) and SyntroPython.isClearToSend(servicePort)):
            # send the frame
            pcm = str("")
            SyntroPython.sendJpegAVData(servicePort, image, pcm)

# start SyntroPython running
SyntroPython.start("SPPicam", sys.argv, True)

# this delay is necessary to allow Qt startup to complete
time.sleep(1)

# put console into single character mode
SyntroPython.startConsoleInput()

# set the title if in GUI mode
SyntroPython.setWindowTitle(SyntroPython.getAppName() + " camera stream")

# wake up the console
print("SPPicam starting...")
print("Enter command: "),
sys.stdout.flush()

# Activate the stream
servicePort = SyntroPython.addMulticastSource(SyntroPythonPy.SYNTRO_STREAMNAME_AVMUX)
if (servicePort == -1):
    print("Failed to activate video service")
    SyntroPython.stop()
    sys.exit()

# Tell the library what video params we are using
SyntroPython.setVideoParams(CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_RATE)

with picamera.PiCamera(CAMERA_INDEX) as camera:
    camera.resolution = (CAMERA_WIDTH, CAMERA_HEIGHT)
    camera.framerate = (CAMERA_RATE, 1)

    # need enough buffering to overcome any latency
    stream = picamera.PiCameraCircularIO(camera, seconds = 1)

    # start recoding in mjpeg mode
    camera.start_recording(stream, format = 'mjpeg')

    try:
        while(True):
            # main loop
            camera.wait_recording(0)
            sendFrame(stream)
                   
            # process any user console input that might have arrived.        
            c = SyntroPython.getConsoleInput()
            if (c != None):
                print("\n")
                sc = chr(c)
                if (sc == "x"):
                    break
                elif (sc == "s"):
                    print("Status:")
                    if (SyntroPython.isConnected()):
                        print("SyntroLink connected")
                    else:
                        print("SyntroLink closed")
                elif (sc == 'h'):
                    print("Available commands are:")
                    print("  s - display status")
                    print("  x - exit")
            
                print("\nEnter command: "),
                sys.stdout.flush()
    finally:
        camera.stop_recording()

# Exiting so clean everything up.    

SyntroPython.removeService(servicePort)
SyntroPython.stop()
print("Exiting")
