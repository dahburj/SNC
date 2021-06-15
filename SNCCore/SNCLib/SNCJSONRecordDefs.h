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

#ifndef _SNCJSONRECORDDEFS_H_
#define _SNCJSONRECORDDEFS_H_

#include "SNCDefs.h"

typedef struct
{
    SNC_RECORD_HEADER recordHeader;                   // the record type header
    SNC_UC4 jsonSize;                                 // size of the json part
    SNC_UC4 binSize;                                  // size of the binary part ((if present))can be 0)
} SNC_RECORD_JSON;

// variables used in JSON SPE messages

#define SNCJSONRECORD_MESSAGE_TYPE                    "type"          // message type
#define SNCJSONRECORD_MESSAGE_TIMESTAMP               "timestamp"     // seconds since epoch (as double)
#define SNCJSONRECORD_MESSAGE_SOURCEMODULE            "source"        // the module that generated the message

// SNCJSONRECORD_MESSAGE_TYPE values

#define SNCJSONRECORD_MESSAGE_TYPE_VIDEOFRAME         "videoframe"    // video frame
#define SNCJSONRECORD_MESSAGE_TYPE_DEPTHFRAME         "depthframe"    // video frame with depth
#define SNCJSONRECORD_MESSAGE_TYPE_AUDIOSAMPLES       "audiosamples"  // audio samples
#define SNCJSONRECORD_MESSAGE_TYPE_ENVSENSOR          "envsensor"     // environmental sensor data
#define SNCJSONRECORD_MESSAGE_TYPE_TEXT               "text"          // arbitrary text
#define SNCJSONRECORD_MESSAGE_TYPE_OPENPOSE           "openpose"      // OpenPose metadata
#define SNCJSONRECORD_MESSAGE_TYPE_UNITYOPENPOSE      "unityopenpose" // Unity OpenPose metadata

// variables used in JSON video records

#define SNCJSONRECORD_VIDEO_WIDTH                     "vfwidth"       // video frame width
#define SNCJSONRECORD_VIDEO_HEIGHT                    "vfheight"      // video frame height
#define SNCJSONRECORD_VIDEO_RATE                      "vfrate"        // video frame rate
#define SNCJSONRECORD_VIDEO_FORMAT                    "vfformat"      // video frame format (eg mjpeg)
#define SNCJSONRECORD_VIDEO_METADATA                  "vfmetadata"    // video frame metadata
#define SNCJSONRECORD_VIDEO_LEFTSIZE                  "vfleftsize"    // left image size (if stereo)
#define SNCJSONRECORD_VIDEO_RIGHTSIZE                 "vfrightsize"   // right image size (if stereo)
#define SNCJSONRECORD_VIDEO_DEPTHSIZE                 "vfdepthsize"   // depth image size (if depth)
#define SNCJSONRECORD_VIDEO_DEPTHFORMAT               "vfdepthformat" // format code
#define SNCJSONRECORD_VIDEO_DEPTHWIDTH                "vfdepthwidth"  // depth frame width
#define SNCJSONRECORD_VIDEO_DEPTHHEIGHT               "vfdepthheight" // depth frame height
#define SNCJSONRECORD_VIDEO_HEIGHT                    "vfheight"      // video frame height
#define SNCJSONRECORD_VIDEO_DEPTHSCALE                "vfdepthscale"  // float to scale depth pixels to meters
#define SNCJSONRECORD_VIDEO_FOVH                      "vffovh"        // horizontal FOV in degrees
#define SNCJSONRECORD_VIDEO_FOVV                      "vffovv"        // vertical FOV in degrees

#define SNCJSONRECORD_VIDEO_FORMAT_MJPEG              "VideoMJPEG"
#define SNCJSONRECORDE_VIDEO_FORMAT_RGBA8888          "VideoRGBA8888"
#define SNCJSONRECORD_VIDEO_FORMAT_DEPTHMJPEG         "DepthMJPEG"
#define SNCJSONRECORD_VIDEO_FORMAT_DEPTHRGBA8888      "DepthRGBA8888"

#define SNCJSONRECORD_VIDEO_DEPTHFORMAT_UINT8         "DepthFormatUint8"  // one byte per depth pixel
#define SNCJSONRECORD_VIDEO_DEPTHFORMAT_UINT16        "DepthFormatUint16" // two bytes per depth pixel
#define SNCJSONRECORD_VIDEO_DEPTHFORMAT_FLOAT32       "DepthFormatFloat32"  // one float per depth pixel

// variables used in JSON audio records

#define SNCJSONRECORD_AUDIO_RATE                      "arate"         // audio sample rate
#define SNCJSONRECORD_AUDIO_CHANNELS                  "achannels"     // number of audio channels
#define SNCJSONRECORD_AUDIO_SAMPTYPE                  "asamptype"     // sample type (eg int16)
#define SNCJSONRECORD_AUDIO_FORMAT                    "aformat"

// variables used in JSON environmental sensor records

#define SNCJSONRECORD_ENVSENSOR_TIMESTAMP             "timestamp"      // seconds since epoch (as double)
#define SNCJSONRECORD_ENVSENSOR_ACCEL_DATA            "accel"          // light value in lumens
#define SNCJSONRECORD_ENVSENSOR_LIGHT                 "light"          // light value in lumens
#define SNCJSONRECORD_ENVSENSOR_TEMPERATURE           "temperature"    // temperature in degress C
#define SNCJSONRECORD_ENVSENSOR_HUMIDITY              "humidity"       // humidity in % RH
#define SNCJSONRECORD_ENVSENSOR_PRESSURE              "pressure"       // pressure in hPa
#define SNCJSONRECORD_ENVSENSOR_AIRQUALITY            "airquality"     // air quality as percent

// variables used in JSON text records

#define SNCJSONRECORD_TEXT_TEXT                       "text"          // an arbitrary string


#endif // _SNCJSONRECORDDEFS_H_
