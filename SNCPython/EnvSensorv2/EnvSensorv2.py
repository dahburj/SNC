#!/usr/bin/python
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

BME680 code via Pimoroni:

MIT License

Copyright (c) 2018 Pimoroni Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
'''

# camera parameters - change as required

CAMERA_ENABLE = False

# If CAMERA_USB is true, expects a webcam
# If False, expects a Pi camera
CAMERA_USB = True

CAMERA_INDEX = 0
CAMERA_WIDTH = 1280
CAMERA_HEIGHT = 720
CAMERA_RATE = 10

import sys
import SNCPython
import SNCPythonPy
import time
import io
import picamera
import json


# import the sensor drivers
sys.path.append('../SensorDrivers')

import SensorJSON

import bme680
import RT_TSL2561
import RT_TSL2591
from RT_I2C import RT_I2C

# the sensor update interval (in seconds). Change as required.
SENSOR_UPDATE_INTERVAL = 0.5

# The set of sensors. Choose which ones are required or use NullSensor if no physical sensor
# Multi sensor objects (such as BMP180 for temp and pressure) can be reused


light = None

if RT_I2C.checkI2CAddress(RT_TSL2591.TSL2591_ADDR):
    light = RT_TSL2591.RT_TSL2591()
    
if RT_I2C.checkI2CAddress(RT_TSL2561.TSL2561_ADDRESS_ADDR_FLOAT):
    light = RT_TSL2561.RT_TSL2561()
    
if light != None:
    light.enable()
    
bmeSensor = bme680.BME680(0x77)
bmeSensor.set_humidity_oversample(bme680.OS_2X)
bmeSensor.set_pressure_oversample(bme680.OS_4X)
bmeSensor.set_temperature_oversample(bme680.OS_8X)
bmeSensor.set_filter(bme680.FILTER_SIZE_3)
bmeSensor.set_gas_status(bme680.ENABLE_GAS_MEAS)

bmeSensor.set_gas_heater_temperature(320)
bmeSensor.set_gas_heater_duration(150)
bmeSensor.select_gas_heater_profile(0)

start_time = time.time()
curr_time = time.time()
burn_in_time = 300
burningIn = True
gas_baseline = 0 
hum_baseline = 0

burn_in_data = []

air_quality_score = -1

'''
------------------------------------------------------------
    User interface function
'''

def processUserInput():
    ''' Process any user console input that might have arrived. 
        Returns False if need to exit. '''
    
    # see if in daemon mode. if so, just return
    if SNCPython.checkDaemonMode():
        return True
    c = SNCPython.getConsoleInput()
    if (c != None):
        print("\n")
        sc = chr(c)
        if (sc == "x"):
            return False
        elif (sc == "s"):
            print("Status:")
            if (SNCPython.isConnected()):
                print("SNCLink connected")
            else:
                print("SNCLink closed")

            print("Temperature: %.2fC" % bmeSensor.data.temperature)
            print("Pressure: %.2fhPa" % bmeSensor.data.pressure)
            print("Humidity: %.2f%%RH" % bmeSensor.data.humidity)
            if light != None:
                print("Light: %.2flux" % light.readLight())
            
            if burningIn:
                print("Burning in gas: {0} Ohms, {1} seconds left".format(bmeSensor.data.gas_resistance, burn_in_time - (curr_time - start_time)))
            else:
                print("Gas baseline: {0} Ohms, humidity baseline: {1:.2f} %RH".format(gas_baseline, hum_baseline))
                print("Gas: {0:.2f} Ohms, humidity: {1:.2f} %RH,air quality: {2:.2f}".format(bmeSensor.data.gas_resistance, bmeSensor.data.humidity, air_quality_score))

        elif (sc == 'h'):
            print("Available commands are:")
            print("  s - display status")
            print("  x - exit")
            
        print("\nEnter command: "),
        sys.stdout.flush()
    return True
    
'''
------------------------------------------------------------
    Sensor functions
'''

# global to maintain last sensor read time
lastSensorReadTime = time.time() 

# global to maintain the sensor service port
sensorPort = -1

def readSensors():
    global lastSensorReadTime
    global sensorPort 
    global bmeSensor, burningIn, burn_in_data, curr_time, burn_in_time, start_time, air_quality_score
    global gas_baseline, hum_baseline, hum_offset, hum_weighting, hum_score
    
    if ((time.time() - lastSensorReadTime) < SENSOR_UPDATE_INTERVAL):
        return
        
    # time send send the sensor readings    
    lastSensorReadTime = time.time()
    
    if bmeSensor.get_sensor_data():
        
        if burningIn:
            if curr_time - start_time < burn_in_time:
                curr_time = time.time()
                if bmeSensor.data.heat_stable:
                    gas = bmeSensor.data.gas_resistance
                    burn_in_data.append(gas)
                    
            else:
                burningIn = False
                gas_baseline = sum(burn_in_data[-50:]) / 50.0

                # Set the humidity baseline to 40%, an optimal indoor humidity.
                hum_baseline = 40.0

                # This sets the balance between humidity and gas reading in the 
                # calculation of air_quality_score (25:75, humidity:gas)
                hum_weighting = 0.25
                        
        else:
            if bmeSensor.data.heat_stable:
                gas = bmeSensor.data.gas_resistance
                gas_offset = gas_baseline - gas

                hum = bmeSensor.data.humidity
                hum_offset = hum - hum_baseline

                # Calculate hum_score as the distance from the hum_baseline.
                if hum_offset > 0:
                    hum_score = (100 - hum_baseline - hum_offset) / (100 - hum_baseline) * (hum_weighting * 100)
                else:
                    hum_score = (hum_baseline + hum_offset) / hum_baseline * (hum_weighting * 100)

                # Calculate gas_score as the distance from the gas_baseline.
                if gas_offset > 0:
                    gas_score = (gas / gas_baseline) * (100 - (hum_weighting * 100))
                else:
                    gas_score = 100 - (hum_weighting * 100)

                # Calculate air_quality_score. 
                air_quality_score = hum_score + gas_score
            
        sensorDict = {}
    
        sensorDict[SensorJSON.TIMESTAMP] = time.time()
        sensorDict[SensorJSON.TEMPERATURE_DATA] = bmeSensor.data.temperature
        sensorDict[SensorJSON.PRESSURE_DATA] = bmeSensor.data.pressure          
        sensorDict[SensorJSON.HUMIDITY_DATA] = bmeSensor.data.humidity
        sensorDict[SensorJSON.AIRQUALITY_DATA] = air_quality_score
        if light != None:
            sensorDict[SensorJSON.LIGHT_DATA] = light.readLight()

        if (SNCPython.isServiceActive(sensorPort) and SNCPython.isClearToSend(sensorPort)):
            SNCPython.sendSensorData(sensorPort, json.dumps(sensorDict))    


'''
------------------------------------------------------------
    USB camera functions and main loop
'''

def USBCameraLoop():
    ''' This is the main loop when the USB camera is enabled. '''
    
    # Open the camera device
    if (not SNCPython.vidCapOpen(CAMERA_INDEX, CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_RATE)):
        print("Failed to open USB camera")
        SNCPython.stop()
        sys.exit()

    # Activate the video stream
    videoPort = SNCPython.addMulticastSource(SNCPythonPy.SNC_STREAMNAME_AVMUX)
    if (videoPort == -1):
        print("Failed to activate video service")
        SNCPython.stop()
        sys.exit()

    while True:
        # try to get a frame from the USB camera
        ret, frame, jpeg, width, height, rate = SNCPython.vidCapGetFrame(CAMERA_INDEX)
        if (ret):            
            # check if it can be sent on the SNCLink
            if (SNCPython.isServiceActive(videoPort) and SNCPython.isClearToSend(videoPort)):
                # send the frame
                SNCPython.setVideoParams(width, height, rate)
                pcm = str("")
                if (jpeg):
                    SNCPython.sendJpegAVData(videoPort, frame, pcm)
                else:
                    SNCPython.sendAVData(videoPort, frame, pcm)
                    
        # check if anything from the sensors            
        readSensors()
        
        # handle user input
        if (not processUserInput()):
            break;
            
        #give other things a chance
        time.sleep(0.02)
               
    # exiting            
    SNCPython.removeService(videoPort)

'''
------------------------------------------------------------
    pi camera functions and main loop
'''

# this is used to track when we have a new frame
piCameraLastFrameIndex = -1

def piCameraSendFrameHelper(stream, frame, videoPort):
    ''' do the actual frame processing '''
    global piCameraLastFrameIndex
    
    # record index of new frame
    piCameraLastFrameIndex = frame.index

    # get the frame data and display it
    stream.seek(frame.position)
    image = stream.read(frame.frame_size)

    # now check if it can be sent on the SNCLink
    if (SNCPython.isServiceActive(videoPort) and SNCPython.isClearToSend(videoPort)):
        # send the frame
        pcm = str("")
        SNCPython.sendJpegAVData(videoPort, image, pcm)
    

def piCameraSendFrame(stream, videoPort):
    ''' sendFrame checks the circular buffer to see if there is a new frame
    and send it on the SNCLink if that's possible '''

    global piCameraLastFrameIndex

    with stream.lock:
        if (CAMERA_RATE > 10):
            for frame in stream.frames:
                if (frame.index > piCameraLastFrameIndex):
                    piCameraSendFrameHelper(stream, frame, videoPort)
        else:
            # skip to last frame in iteration
            frame = None
            for frame in stream.frames:
                pass
 
            if (frame is None):
                return
         
            # check to see if this is new
            if (frame.index == piCameraLastFrameIndex):
                return
            piCameraSendFrameHelper(stream, frame, videoPort)
        
 
def piCameraLoop():
    ''' This is the main loop when the pi camera is enabled. '''
    # Activate the video stream
    videoPort = SNCPython.addMulticastSource(SNCPythonPy.SNC_STREAMNAME_AVMUX)
    if (videoPort == -1):
        print("Failed to activate video service")
        SNCPython.stop()
        sys.exit()

    # Tell the library what video params we are using
    SNCPython.setVideoParams(CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_RATE)

    with picamera.PiCamera(CAMERA_INDEX) as camera:
        camera.resolution = (CAMERA_WIDTH, CAMERA_HEIGHT)
        camera.framerate = (CAMERA_RATE, 1)

        # need enough buffering to overcome any latency
        stream = picamera.PiCameraCircularIO(camera, seconds = 1)

        # start recoding in mjpeg mode
        camera.start_recording(stream, format = 'mjpeg')

        try:
            while(True):
                # process any new frames
                camera.wait_recording(0)
                piCameraSendFrame(stream, videoPort)
                
                # see if anythng new from the sensors
                readSensors()
                
                # handle user input
                if (not processUserInput()):
                    break;
                    
                #give other things a chance
                time.sleep(0.02)
  
        finally:
            camera.stop_recording()
            SNCPython.removeService(videoPort)

'''
------------------------------------------------------------
    No camera main loop
'''


def noCameraLoop():
    ''' This is the main loop when no camera is running. '''

    while True:
        # see if anything from the sensors
        readSensors()
        
        # handle any user input
        if (not processUserInput()):
            break;
            
        # give other things a chance
        time.sleep(0.02)

'''
------------------------------------------------------------
    Main code
'''   
time.sleep(30)

# start SNCPython running
SNCPython.start("EnvSensorv2", sys.argv, False)

# this delay is necessary to allow Qt startup to complete
time.sleep(2)

if not SNCPython.checkDaemonMode():
    # put console into single character mode
    SNCPython.startConsoleInput()
    print("EnvSensorv2 starting...")
    print("Enter command: "),
    sys.stdout.flush()
    
# set the title if in GUI mode
SNCPython.setWindowTitle(SNCPython.getAppName() + " camera stream")

sensorPort = SNCPython.addMulticastSource(SNCPythonPy.SNC_STREAMNAME_SENSOR)
if (sensorPort == -1):
    print("Failed to activate sensor service")
    SNCPython.stop()
    sys.exit()
        
if CAMERA_ENABLE:
    if CAMERA_USB:
        USBCameraLoop()
    else:
        piCameraLoop()
else:
    noCameraLoop()

# Exiting so clean everything up.    

SNCPython.stop()
if not SNCPython.checkDaemonMode():
    print("EnvSensorv2 exiting")
