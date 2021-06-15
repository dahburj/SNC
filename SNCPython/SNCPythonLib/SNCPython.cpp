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

#include <Python.h>
#include "SNCPythonGlue.h"
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

static pthread_t syPyThread;
static SNCPythonGlue syPyGlue;
static int argc;
static char **argv;
static bool showWindow;

void *runThread(void *appName)
{
    syPyGlue.startLib((const char *)appName, argc, argv, showWindow);
    pthread_exit(0);
}

static PyObject *start(PyObject *self, PyObject *args)
{
    char *appName;
    PyObject *cmdArgs;

    if (!PyArg_ParseTuple(args, "sO!b", &appName, &PyList_Type, &cmdArgs, &showWindow)) {
        printf("Bad argument to start\n");
        Py_RETURN_NONE;
    }

    argc = PyList_Size(cmdArgs);
    argv = (char **)malloc(argc * sizeof(char *));

    for (int i = 0; i < argc; i++) {
        const char *arg = PyUnicode_AsUTF8(PyList_GetItem(cmdArgs, i));
        argv[i] = (char *)malloc(strlen(arg) + 1);
        strcpy(argv[i], arg);
    }

    pthread_create(&syPyThread, NULL, runThread, appName);

    Py_RETURN_NONE;
}

static PyObject *stop(PyObject *self, PyObject *args)
{
    void *threadRes;
    syPyGlue.stopLib();
    pthread_join(syPyThread, &threadRes);
    Py_RETURN_NONE;
}

static PyObject *checkDaemonMode(PyObject *self, PyObject *args)
{
    bool mode = syPyGlue.checkDaemonMode();

    return Py_BuildValue("i", mode);
}
static PyObject *setWindowTitle(PyObject *self, PyObject *args)
{
    char *title;

    if (!PyArg_ParseTuple(args, "s", &title)) {
        printf("Bad argument to setWindowTitle\n");
        Py_RETURN_NONE;
    }

    syPyGlue.setWindowTitle(title);
    Py_RETURN_NONE;
}

static PyObject *displayImage(PyObject *self, PyObject *args)
{
    unsigned char *image;
    int length;
    int width;
    int height;
    char *timestamp;

    if (!PyArg_ParseTuple(args, "s#iis", &image, &length, &width, &height, &timestamp)) {
        printf("Bad argument to displayImage\n");
        Py_RETURN_NONE;
    }

    syPyGlue.displayImage(image, length, width, height, timestamp);
    Py_RETURN_NONE;
}

static PyObject *displayJpegImage(PyObject *self, PyObject *args)
{
    unsigned char *image;
    int length;
    char *timestamp;

    if (!PyArg_ParseTuple(args, "s#s", &image, &length, &timestamp)) {
        printf("Bad argument to displayJpegImage\n");
        Py_RETURN_NONE;
    }

    syPyGlue.displayJpegImage(image, length, timestamp);
    Py_RETURN_NONE;
}

static PyObject *vidCapOpen(PyObject *self, PyObject *args)
{
    int cameraNum;
    int width;
    int height;
    int rate;

    if (!PyArg_ParseTuple(args, "iiii", &cameraNum, &width, &height, &rate)) {
        printf("Bad argument to vidCapOpen\n");
        return Py_BuildValue("i", 0);
    }

    return Py_BuildValue("i", syPyGlue.vidCapOpen(cameraNum, width, height, rate));
}

static PyObject *vidCapClose(PyObject *self, PyObject *args)
{
    int cameraNum;

    if (!PyArg_ParseTuple(args, "i", &cameraNum)) {
        printf("Bad argument to vidCapClose\n");
        return Py_BuildValue("i", 0);
    }

    return Py_BuildValue("i", syPyGlue.vidCapClose(cameraNum));
}

static PyObject *vidCapGetFrame(PyObject *self, PyObject *args)
{
    int cameraNum;
    int width = 0;
    int height = 0;
    int rate = 0;
    bool jpeg = false;
    unsigned char *frame = NULL;
    int length = 0;

    if (!PyArg_ParseTuple(args, "i", &cameraNum)) {
        printf("Bad argument to vidCapGetFrame\n");
        return Py_BuildValue("is#iiii", false, frame, length,
                             jpeg, width, height, rate);
    }
    bool ret = syPyGlue.vidCapGetFrame(cameraNum, &frame, length, jpeg, width, height, rate);
    PyObject* retData = Py_BuildValue("iy#iiii", ret, frame, length,
                                      jpeg, width, height, rate);
    if (frame != NULL)
        free(frame);
    return retData;
}

static PyObject *startConsoleInput(PyObject *self, PyObject *args)
{
    struct termios	ctty;

    tcgetattr(fileno(stdout), &ctty);
    ctty.c_lflag &= ~(ICANON);
    tcsetattr(fileno(stdout), TCSANOW, &ctty);
    Py_RETURN_NONE;
}

static PyObject *getConsoleInput(PyObject *self, PyObject *args)
{
    int i;

    ioctl(0, FIONREAD, &i);
    if (i <= 0)
        Py_RETURN_NONE;

    return Py_BuildValue("b", getchar());
}


static PyObject *getAppName(PyObject *self, PyObject *args)
{
    return Py_BuildValue("s", syPyGlue.getAppName());
}

static PyObject *isConnected(PyObject *self, PyObject *args)
{
    if (syPyGlue.isConnected())
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *requestDirectory(PyObject *self, PyObject *args)
{
    if (syPyGlue.requestDirectory())
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *lookupMulticastSources(PyObject *self, PyObject *args)
{
    char *serviceName;

    if (!PyArg_ParseTuple(args, "s", &serviceName)) {
        printf("Bad argument to lookupMulticastSources\n");
        return Py_BuildValue("s", "");
    }

    return Py_BuildValue("s", syPyGlue.lookupMulticastSources(serviceName));
}

static PyObject *addMulticastSource(PyObject *self, PyObject *args)
{
    char *servicePath;

    if (!PyArg_ParseTuple(args, "s", &servicePath)) {
        printf("Bad argument to addMulticastSource\n");
        return Py_BuildValue("i", -1);
    }
    return Py_BuildValue("i", syPyGlue.addMulticastSourceService(servicePath));
}

static PyObject *addMulticastSink(PyObject *self, PyObject *args)
{
    char *servicePath;

    if (!PyArg_ParseTuple(args, "s", &servicePath)) {
        printf("Bad argument to addMulticastSink\n");
        return Py_BuildValue("i", -1);
    }
    return Py_BuildValue("i", syPyGlue.addMulticastSinkService(servicePath));
}

static PyObject *lookupE2ESources(PyObject *self, PyObject *args)
{
    char *serviceName;

    if (!PyArg_ParseTuple(args, "s", &serviceName)) {
        printf("Bad argument to lookupE2ESources\n");
        return Py_BuildValue("s", "");
    }

    return Py_BuildValue("s", syPyGlue.lookupE2ESources(serviceName));
}

static PyObject *addE2ESource(PyObject *self, PyObject *args)
{
    char *servicePath;

    if (!PyArg_ParseTuple(args, "s", &servicePath)) {
        printf("Bad argument to addE2ESource\n");
        return Py_BuildValue("i", -1);
    }
    return Py_BuildValue("i", syPyGlue.addE2ESourceService(servicePath));
}

static PyObject *addE2ESink(PyObject *self, PyObject *args)
{
    char *servicePath;

    if (!PyArg_ParseTuple(args, "s", &servicePath)) {
        printf("Bad argument to addE2ESink\n");
        return Py_BuildValue("i", -1);
    }
    return Py_BuildValue("i", syPyGlue.addE2ESinkService(servicePath));
}


static PyObject *removeService(PyObject *self, PyObject *args)
{
    int servicePort;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to removeService\n");
        return Py_BuildValue("i", 0);
    }
    return Py_BuildValue("i", syPyGlue.removeService(servicePort));
}

static PyObject *isClearToSend(PyObject *self, PyObject *args)
{
    int servicePort;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to isClearToSend\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.isClearToSend(servicePort))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *isServiceActive(PyObject *self, PyObject *args)
{
    int servicePort;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to isServiceActive\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.isServiceActive(servicePort))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *sendAVData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *videoData;
    int videoLength;
    unsigned char *audioData;
    int audioLength;

    if (!PyArg_ParseTuple(args, "iy#y#", &servicePort, &videoData, &videoLength, &audioData, &audioLength)) {
        printf("Bad argument to sendAVData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendAVData(servicePort, videoData, videoLength, audioData, audioLength))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *sendJpegAVData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *videoData;
    int videoLength;
    unsigned char *audioData;
    int audioLength;

    if (!PyArg_ParseTuple(args, "iy#y#", &servicePort, &videoData, &videoLength, &audioData, &audioLength)) {
        printf("Bad argument to sendJpegAVData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendJpegAVData(servicePort, videoData, videoLength, audioData, audioLength))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *getAVData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *videoData = NULL;
    int videoLength = 0;
    unsigned char *audioData = NULL;
    int audioLength = 0;
    long long timestamp = 0;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to getAVData\n");
        return Py_BuildValue("iLy#y#", false, timestamp, videoData, videoLength, audioData, audioLength);
    }
    bool ret = syPyGlue.getAVData(servicePort, &timestamp, &videoData, &videoLength, &audioData, &audioLength);
    PyObject* retData = Py_BuildValue("iLy#y#", ret, timestamp, videoData, videoLength, audioData, audioLength);
    if (videoData != NULL)
        free(videoData);
    if (audioData != NULL)
        free(audioData);
    return retData;
}

static PyObject *getVideoData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *videoData = NULL;
    int videoLength = 0;
    int width = 0;
    int height = 0;
    int rate = 0;
    long long timestamp = 0;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to getVideoData\n");
        return Py_BuildValue("iLy#iii", false, timestamp, videoData, videoLength, width, height, rate);
    }
    bool ret = syPyGlue.getVideoData(servicePort, &timestamp, &videoData, &videoLength, &width, &height, &rate);
    PyObject* retData = Py_BuildValue("iLy#iii", ret, timestamp, videoData, videoLength, width, height, rate);
    if (videoData != NULL)
        free(videoData);
    return retData;
}

static PyObject *sendVideoData(PyObject *self, PyObject *args)
{
    int servicePort;
    long long timestamp;
    unsigned char *videoData;
    int videoLength;
    int width;
    int height;
    int rate;

    if (!PyArg_ParseTuple(args, "iLy#iii", &servicePort, &timestamp, &videoData, &videoLength, &width, &height, &rate)) {
        printf("Bad argument to sendVideoData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendVideoData(servicePort, timestamp, videoData, videoLength, width, height, rate))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *setVideoParams(PyObject *self, PyObject *args)
{
    int width, height, rate;

    if (!PyArg_ParseTuple(args, "iii", &width, &height, &rate)) {
        printf("Bad argument to setVideoParams\n");
        Py_RETURN_NONE;
    }

    syPyGlue.setVideoParams(width, height, rate);
    Py_RETURN_NONE;
}

static PyObject *setAudioParams(PyObject *self, PyObject *args)
{
    int rate, size, channels;

    if (!PyArg_ParseTuple(args, "iii", &rate, &size, &channels)) {
        printf("Bad argument to setAudioParams\n");
        Py_RETURN_NONE;
    }

    syPyGlue.setAudioParams(rate, size, channels);
    Py_RETURN_NONE;
}

static PyObject *getSensorData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data = NULL;
    int dataLength = 0;
    long long timestamp = 0;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to getSensorData\n");
        return Py_BuildValue("iLs#", false, timestamp, data, dataLength);
    }
    bool ret = syPyGlue.getSensorData(servicePort, &timestamp, &data, &dataLength);
    PyObject* retData = Py_BuildValue("iLs#", ret, timestamp, data, dataLength);
    if (data != NULL)
        free(data);
    return retData;
}

static PyObject *sendSensorData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data;
    int dataLength;

    if (!PyArg_ParseTuple(args, "is#", &servicePort, &data, &dataLength)) {
        printf("Bad argument to sendSensorData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendSensorData(servicePort, data, dataLength))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *sendMulticastData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data;
    int dataLength;

    if (!PyArg_ParseTuple(args, "iy#", &servicePort, &data, &dataLength)) {
        printf("Bad argument to sendMulticastData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendMulticastData(servicePort, data, dataLength))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *sendE2EData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data;
    int dataLength;

    if (!PyArg_ParseTuple(args, "iy#", &servicePort, &data, &dataLength)) {
        printf("Bad argument to sendE2EData\n");
        return Py_BuildValue("i", 0);
    }

    if (syPyGlue.sendE2EData(servicePort, data, dataLength))
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
}

static PyObject *getMulticastData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data = NULL;
    int dataLength = 0;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to getMulticastData\n");
        return Py_BuildValue("iy#", false, data, dataLength);
    }
    bool ret = syPyGlue.getMulticastData(servicePort, &data, &dataLength);
    PyObject* retData = Py_BuildValue("iy#", ret, data, dataLength);
    if (data != NULL)
        free(data);
    return retData;
}

static PyObject *getE2EData(PyObject *self, PyObject *args)
{
    int servicePort;
    unsigned char *data = NULL;
    int dataLength = 0;

    if (!PyArg_ParseTuple(args, "i", &servicePort)) {
        printf("Bad argument to getE2EData\n");
        return Py_BuildValue("iy#", false, data, dataLength);
    }
    bool ret = syPyGlue.getE2EData(servicePort, &data, &dataLength);
    PyObject* retData = Py_BuildValue("iy#", ret, data, dataLength);
    if (data != NULL)
        free(data);
    return retData;
}


static PyMethodDef SNCPythonMethods[] = {
    {"start", (PyCFunction)start, METH_VARARGS,
    "Starts the SNCPython library.\n"
    "Call this before calling any other SNCPython functions.\n"
    "The function takes three parameters:\n"
    "  appType - a string defining the type of the app\n"
    "  args - the command line args as string (sys.argv)\n"
    "  showWindow - whether to show the GUI window\n"
    "    (ignored in console mode)\n"
    "The function returns None"},

    {"stop", (PyCFunction)stop, METH_NOARGS,
    "Stops the SNCPython library.\n"
    "This should be called just before exiting.\n"
    "There are no parameters and the function returns None."},

    {"checkDaemonMode", (PyCFunction)checkDaemonMode, METH_NOARGS,
    "Returns True if '-d' was specified on command line"},

    {"vidCapOpen", (PyCFunction)vidCapOpen, METH_VARARGS,
    "Tries to open a camera. The camera is specified by a \n"
    "camera number starting from 0.\n"
    "0 is equivalent to /dev/video0, 1 is /dev/video1 etc\n"
    "The function returns true if successful.\n"},

    {"vidCapClose", (PyCFunction)vidCapClose, METH_VARARGS,
    "Tries to close a camera. The camera is specified by a \n"
    "camera number starting from 0.\n"
    "0 is equivalent to /dev/video0, 1 is /dev/video1 etc\n"
    "The function returns true if successful.\n"},

    {"vidCapGetFrame", (PyCFunction)vidCapGetFrame, METH_VARARGS,
    "Tries to get a captured frame from a camera.\n"
    "The function takes the camera number as its parameter.\n"
    "It returns 6 values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  frame - captured frame (could be jpeg or uncompressed)\n"
    "  jpeg - True if frame is jpeg compressed, False for uncompressed\n"
    "  width - width of frame\n"
    "  height - height of frame\n"
    "  rate - frame rate\n"},

    {"setWindowTitle", (PyCFunction)setWindowTitle, METH_VARARGS,
    "Sets the window title in GUI mode.\n"
    "The parameter is a string containing the new window title.\n"
    "The function returns None"},

    {"displayImage", (PyCFunction)displayImage, METH_VARARGS,
    "Displays an uncompressed image in the GUI window.\n"
    "There are four parameters:\n"
    "  image - the image as a string\n"
    "  width - the width of the image\n"
    "  height - the height of the image\n"
    "  timestamp - a string containing the timestamp (can be empty)\n"
    "The function returns None"},

    {"displayJpegImage", (PyCFunction)displayJpegImage, METH_VARARGS,
    "Displays a Jpeg image in the GUI window.\n"
    "There are two parameters:\n"
    "  image - the Jpeg image as a string\n"
    "  timestamp - a string containing the timestamp (can be empty)\n"
    "The function returns None"},

    {"getAppName", (PyCFunction)getAppName, METH_NOARGS,
    "Returns the app name. This is used as the streamName of any \n"
    "stream generated by this app.\n"
    "There are no parameters. The function returns a string \n"
    "containing the app name."},

    {"isConnected", (PyCFunction)isConnected, METH_NOARGS,
    "This function checks if the SNCLink is in the connected state.\n"
    "The function takes no parameters and returns 1 if connected, 0 otherwise"},

    {"requestDirectory", (PyCFunction)requestDirectory, METH_NOARGS,
    "Requests that an updated SNCNet directory be sent to this app.\n"
    "The directory will be requested at the next opportunity and will be\n"
    "received some time after that - the function itself is non-blocking.\n"
    "The function takes no parameters and returns true if the request was\n"
    "queued."},

    {"lookupMulticastSources", (PyCFunction)lookupMulticastSources, METH_VARARGS,
    "This function takes a serviceType as a parameter and returns a list\n"
    "of serviceNames that have published that serviceType as a multicast\n"
    "service. SNCPythonPy.py contains a list of standard serviceTypes\n"
     "although custom ones can be used. An example is SYNTRO_STREAMNAME_AVMUX\n"
     "(yes, confusingly this is called a name).\n"
     "The function returns a string containing end of line separated\n"
     "serviceNames. The list can be empty."},

    {"lookupE2ESources", (PyCFunction)lookupE2ESources, METH_VARARGS,
    "Same as lookupMulticastSources except that E2E sources are returned."},

    {"addMulticastSource", (PyCFunction)addMulticastSource, METH_VARARGS,
    "Activates a multicast source. In other words, it publishes a\n"
    "multicast source. The parameter to the function is a string containing\n"
    "a service name.\n"
    "The function returns the allocated service port (0 or greater) or else\n"
     "-1 if en error occurred."},

    {"addMulticastSink", (PyCFunction)addMulticastSink, METH_VARARGS,
    "Activates a multicast sink. In other words, it subscribes to a\n"
    "multicast source. The parameter to the function is a string containing\n"
    "a service path. A service path is a concatentation of a serviceName,\n"
    "a '/' character and a serviceType. For example:\n"
    "  appname/avmux \n"
    "would describe an AVMUX stream from an app with name appname.\n"
    "The function returns the allocated service port (0 or greater) or else\n"
     "-1 if en error occurred."},

    {"addE2ESource", (PyCFunction)addE2ESource, METH_VARARGS,
    "Activates an E2E source. In other words, it publishes an\n"
    "end to end source. The parameter to the function is a string containing\n"
    "a service name.\n"
    "The function returns the allocated service port (0 or greater) or else\n"
    "-1 if en error occurred."},

    {"addE2ESink", (PyCFunction)addE2ESink, METH_VARARGS,
    "Activates an E2E sink. In other words, it subscribes to an\n"
    "end to end source. The parameter to the function is a string containing\n"
    "a service path. A service path is a concatentation of a serviceName,\n"
    "a '/' character and a serviceType. For example:\n"
    "'appname/control' \n"
    "would describe a CONTROL stream from an app with name appname.\n"
    "The function returns the allocated service port (0 or greater) or else\n"
    "-1 if en error occurred."},

    {"removeService", (PyCFunction)removeService, METH_VARARGS,
    "Removes an active service (source or sink, multicast or E2E)\n"
    "and returns True if ok, False if error. The parameter to the function\n"
    "is the service port return by the original activation."},

    {"isClearToSend", (PyCFunction)isClearToSend, METH_VARARGS,
    "This function is only valid for a multicast source. It is a rate\n"
    "limiting mechanism and it is only possible to send multicast data\n"
    "if the acknowledgement window is open.\n"
    "The function takes the service port as its parameter and returns\n"
    "True if the window is open, False otherwise."},

    {"isServiceActive", (PyCFunction)isServiceActive, METH_VARARGS,
    "This function's meaning depends on the type of service:\n"
    "  multicast source: return sTrue if there is at least one subscriber\n"
    "  multicast sink: returns true if the service path lookup has succeeded\n"
    "  E2E source: not valid (do not use this function)\n"
    "  E2E sink: returns true if the service path lookup has succeeded\n"
    "The function takes the service port as its parameter."},

    {"setVideoParams", (PyCFunction)setVideoParams, METH_VARARGS,
    "This function should be called by an AVMUX source before sending\n"
    "any data. It takes three parameters:\n"
    "  width - the width of the frames\n"
    "  height - the height of the frames\n"
    "  rate - the frame rate\n"
    "There is no returned value."},

    {"setAudioParams", (PyCFunction)setAudioParams, METH_VARARGS,
    "This function should be called by an AVMUX source before sending\n"
    "any data. It takes three parameters:\n"
    "  rate - the sample rate of the audio data\n"
    "  size - the number of bits per sample\n"
    "  channels - the number of channels\n"
    "There is no returned value."},

    {"sendAVData", (PyCFunction)sendAVData, METH_VARARGS,
    "This function sends sends audio and video data in the AVMUX format.\n"
    "The service must be a multicast source that is active and clear to send.\n"
    "The function takes three parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  videoData - a str containing an uncompressed frame\n"
    "  audioData - a str containing uncompressed PCM format audio\n"
    "Returns True if the call succeeded, False otherwise."},

    {"sendJpegAVData", (PyCFunction)sendJpegAVData, METH_VARARGS,
    "This function sends sends audio and video data in the AVMUX format.\n"
    "The service must be a multicast source that is active and clear to send.\n"
    "The function takes three parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  videoData - a str containing a jpeg compressed frame\n"
    "  audioData - a str containing uncompressed PCM format audio\n"
    "Returns True if the call succeeded, False otherwise."},

    {"getAVData", (PyCFunction)getAVData, METH_VARARGS,
    "Gets audio and video data from an AVMUX format multicast source.\n"
    "The function takes the service port as its parameter.\n"
    "It returns four values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  timestamp - the timestamp from the message (mS since epoch)\n"
    "  videoData - any compressed jpeg image received\n"
    "  audioData - any uncompressed PCM audio data received\n"
    "Note that either videoData or audioData could be None."},

    {"getVideoData", (PyCFunction)getVideoData, METH_VARARGS,
    "Gets video data from an AVMUX format multicast source.\n"
    "The function takes the service port as its parameter.\n"
    "It returns five values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  timestamp - the timestamp from the message (mS since epoch)\n"
    "  videoData - any compressed jpeg image received\n"
    "  width - width of frame\n"
    "  height - height of frame\n"
    "  rate - frame rate\n"
    "Note that videoData could be None."},

    {"sendVideoData", (PyCFunction)sendVideoData, METH_VARARGS,
    "This function sends sends video data.\n"
    "The service must be a multicast source that is active and clear to send.\n"
    "The function takes 5 parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  timestamp - timestamp to be used for frame\n"
    "  videoData - a str containing a jpeg video frame\n"
     "  width - width of frame\n"
     "  height - height of frame\n"
     "  rate - framerate\n"
    "Returns True if the call succeeded, False otherwise."},

    {"sendSensorData", (PyCFunction)sendSensorData, METH_VARARGS,
    "This function sends sends sensor multicast data (with a sensor\n"
     " record header. The service must be \n"
    "a multicast source that is active and clear to send.\n"
    "The function takes two parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  data - a str containing the data to be sent\n"
    "Returns True if the call succeeded, False otherwise."},

    {"getSensorData", (PyCFunction)getSensorData, METH_VARARGS,
    "Gets sensor data from a multicast source.\n"
    "The function takes the service port as its parameter.\n"
    "It returns 3 values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  timestamp - the timestamp from the message (mS since epoch)\n"
    "  jsonData - the sensor record\n"},

    {"sendMulticastData", (PyCFunction)sendMulticastData, METH_VARARGS,
    "This function sends sends generic data. The service must be \n"
    "a multicast source that is active and clear to send.\n"
    "The function takes two parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  data - a str containing the data to be sent\n"
    "Returns True if the call succeeded, False otherwise."},

    {"sendE2EData", (PyCFunction)sendE2EData, METH_VARARGS,
    "This function sends sends generic data.\n."
    "The service must be an E2E source or sink that is active.\n"
    "The function takes two parameters:\n"
    "  servicePort - the service port on which to send the data\n"
    "  data - a str containing the data to be sent\n"
    "Returns True if the call succeeded, False otherwise."},

    {"getMulticastData", (PyCFunction)getMulticastData, METH_VARARGS,
    "Gets generic data from a multicast source.\n"
    "The function takes the service port as its parameter.\n"
    "It returns two values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  data - the data contained in the message\n"},

    {"getE2EData", (PyCFunction)getE2EData, METH_VARARGS,
    "Gets generic data from an E2E source.\n"
    "The function takes the service port as its parameter.\n"
    "It returns two values:\n"
    "  valid - True or False depending on whether the call succeeded\n"
    "  data - the data contained in the message\n"},

    {"startConsoleInput", (PyCFunction)startConsoleInput, METH_NOARGS,
    "Configures stdin for single char input."},

    {"getConsoleInput", (PyCFunction)getConsoleInput, METH_NOARGS,
    "Gets a single char from input or None if nothing"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "SNCPython",
        NULL,
        -1,
        SNCPythonMethods,
};

PyMODINIT_FUNC PyInit_SNCPython(void)
{
    return PyModule_Create(&moduledef);
}
