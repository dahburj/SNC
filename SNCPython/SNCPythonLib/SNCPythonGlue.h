/////////////////////////////////////////////////////////////////////////////
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

#ifndef SNCPYTHONGLUE_H
#define SNCPYTHONGLUE_H

class SNCPythonMain;

class SNCPythonGlue
{
public:
    SNCPythonGlue();
    virtual ~SNCPythonGlue();

    //  functions called from Python

    bool vidCapOpen(int cameraNum, int width, int height, int rate);
    bool vidCapClose(int cameraNum);
    bool vidCapGetFrame(int cameraNum, unsigned char** frame, int& length, bool& jpeg,
                                             int& width, int& height, int& rate);

    void startLib(const char *productType, int& argc, char **argv, bool showWindow);
    void stopLib();
    bool checkDaemonMode() { return m_daemonMode;}          // checks if in daemon mode
    void setWindowTitle(char *title);                       // sets the window title in GUI mode
    void displayImage(unsigned char *image, int length,
                      int width, int height, char *timestamp); // displays an image in GUI mode
    void displayJpegImage(unsigned char *image, int length, char *timestamp); // displays a Jpeg in GUI mode
    char *getAppName();                                     // returns the app name
    bool isConnected();                                     // returns true if link connected

    bool requestDirectory();                                // requests directory at next opportunity

    int addMulticastSourceService(const char *servicePath); // adds a multicast source and returns the service port or -1 if error
    int addMulticastSinkService(const char *servicePath);   // adds a multicast sink and returns the service port or -1 if error
    int addE2ESourceService(const char *servicePath);       // adds a E2E source and returns the service port or -1 if error
    int addE2ESinkService(const char *servicePath);         // adds a E2E sink and returns the service port or -1 if error
    bool removeService(int servicePort);                    // removes the service associated with servicePort

    char *lookupMulticastSources(const char *sourceType);   // gets a list of multicast sources of the specified type
    char *lookupE2ESources(const char *sourceType);         // gets a list of E2E sources of the specified type

    bool isClearToSend(int servicePort);                    // returns true if window open on multicast source service
    bool isServiceActive(int servicePort);                  // returns true if service is active (multicast source or remote E2E)

    void setVideoParams(int width, int height, int rate);   // sets the video capture data
    void setAudioParams(int rate, int size, int channels);  // sets the audio capture data
    bool sendAVData(int servicePort, unsigned char *videoData, int videoLength,
                        unsigned char *audioData, int audioLength); // sends an AV data message
    bool sendJpegAVData(int servicePort, unsigned char *videoData, int videoLength,
                        unsigned char *audioData, int audioLength); // sends an AV data message
    bool getAVData(int servicePort, long long *timestamp, unsigned char **videoData, int *videoLength,
                        unsigned char **audioData, int *audioLength); // gets AV data if available

    bool getVideoData(int servicePort, long long *timestamp, unsigned char **videoData, int *videoLength,
                        int *width, int *height, int *rate); // gets video data if available
    bool sendVideoData(int servicePort, long long timestamp, unsigned char *videoData, int videoLength,
                        int width, int height, int rate);   // send video data

    bool getSensorData(int servicePort, long long *timestamp, unsigned char **jsonData, int *jsonLength); // gets json data if available
    bool sendSensorData(int servicePort, unsigned char *data, int dataLength); // sends a sensor multicast message
    bool sendMulticastData(int servicePort, unsigned char *data, int dataLength); // sends a generic multicast message
    bool sendE2EData(int servicePort, unsigned char *data, int dataLength); // sends a generic E2E message

    bool getMulticastData(int servicePort, unsigned char **data, int *dataLength);  // get generic multicast data
    bool getE2EData(int servicePort, unsigned char **data, int *dataLength);  // get generic E2E data

    //  functions called by client

    void setConnected(bool state);                          // sets whether link connected or not
    bool isDirectoryRequested();                            // true if a directory has been requested

private:
    SNCPythonMain *m_main;

    bool m_connected;
    bool m_directoryRequested;
    bool m_daemonMode;

    int m_argc;
    char **m_argv;
};

#endif // SNCPYTHONGLUE_H

