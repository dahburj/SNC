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

#ifndef _SNCAVDEFS_H
#define _SNCAVDEFS_H

#include "SNCDefs.h"

//  Record header parameter field values

typedef enum
{
    SNC_RECORDHEADER_PARAM_NOOP = 0,                        // indicates a filler record
    SNC_RECORDHEADER_PARAM_REFRESH,                         // indicates a refresh MJPEG frame
    SNC_RECORDHEADER_PARAM_NORMAL,                          // indicates a normal record
    SNC_RECORDHEADER_PARAM_PREROLL,                         // indicates a preroll frame
    SNC_RECORDHEADER_PARAM_POSTROLL,                        // indicates a postroll frame
 } SNCAV_RECORDHEADER_PARAM;

//  AVMux subType codes

typedef enum
{
    SNC_RECORD_TYPE_AVMUX_UNKNOWN = -1,                     // unknown mux
    SNC_RECORD_TYPE_AVMUX_MJPPCM,                           // MJPEG + PCM interleaved
    SNC_RECORD_TYPE_AVMUX_MP4,                              // MP4 mux
    SNC_RECORD_TYPE_AVMUX_OGG,                              // Ogg mux
    SNC_RECORD_TYPE_AVMUX_WEBM,                             // Webm mux
    SNC_RECORD_TYPE_AVMUX_RTP,                              // RTP interleave format
    SNC_RECORD_TYPE_AVMUX_RTPCAPS,                          // RTP caps

    // This entry marks the end of the enum

    SNC_RECORD_TYPE_AVMUX_END                               // the end
} SNCAV_AVMUXSUBTYPE;


//	Video subType codes

typedef enum
{
    SNC_RECORD_TYPE_VIDEO_UNKNOWN = -1,                     // unknown format or not present
    SNC_RECORD_TYPE_VIDEO_MJPEG,                            // MJPEG compression
    SNC_RECORD_TYPE_VIDEO_MPEG1,                            // MPEG1 compression
    SNC_RECORD_TYPE_VIDEO_MPEG2,                            // MPEG2 compression
    SNC_RECORD_TYPE_VIDEO_H264,                             // H264 compression
    SNC_RECORD_TYPE_VIDEO_VP8,                              // VP8 compression
    SNC_RECORD_TYPE_VIDEO_THEORA,                           // theora compression
    SNC_RECORD_TYPE_VIDEO_RTPMPEG4,                         // mpeg 4 over RTP
    SNC_RECORD_TYPE_VIDEO_RTPCAPS,                          // RTP caps message
    SNC_RECORD_TYPE_VIDEO_RTPH264,                          // h.264 over RTP

    // This entry marks the end of the enum

    SNC_RECORD_TYPE_VIDEO_END                               // the end
} SNCAV_VIDEOSUBTYPE;

//	Audio subType codes

typedef enum
{
    SNC_RECORD_TYPE_AUDIO_UNKNOWN = -1,                     // unknown format or not present
    SNC_RECORD_TYPE_AUDIO_PCM,                              // PCM/WAV uncompressed
    SNC_RECORD_TYPE_AUDIO_MP3,                              // MP3 compression
    SNC_RECORD_TYPE_AUDIO_AC3,                              // AC3 compression
    SNC_RECORD_TYPE_AUDIO_AAC,                              // AAC compression
    SNC_RECORD_TYPE_AUDIO_VORBIS,                           // Vorbis compression
    SNC_RECORD_TYPE_AUDIO_RTPAAC,                           // aac over RTP
    SNC_RECORD_TYPE_AUDIO_RTPCAPS,                          // RTP caps message

    // This entry marks the end of the enum

    SNC_RECORD_TYPE_AUDIO_END                               // the end
} SNCAV_AUDIOSUBTYPE;

//  SNC_RECORD_AVMUX - used to send muxed video and audio data
//
//  The structure follows the SNC_EHEAD structure. The lengths of the
//  three types of data (avmux, video and audio) describe what is present. The
//  actual data is always in that order. So, if there's video and audio, the video would
//  be first and the audio second. The offset to the audio can be calculated from the
//  length of the video.
//
//  The header information is always set correctly regardless of what data is contained
//  so that any record can be used to determine the types of data that the stream carries.

typedef struct
{
    SNC_RECORD_HEADER recordHeader;                         // the record type header
    SNC_UC2 spare0;                                         // unused at present - set to 0
    unsigned char videoSubtype;                             // type of video stream
    unsigned char audioSubtype;                             // type of audio stream
    SNC_UC4 muxSize;                                        // size of the muxed data (if present)
    SNC_UC4 videoSize;                                      // size of the video data (if present)
    SNC_UC4 audioSize;                                      // size of the audio data (if present)
    SNC_UC2 videoWidth;                                     // width of frame
    SNC_UC2 videoHeight;                                    // height of frame
    SNC_UC2 videoFramerate;                                 // framerate
    SNC_UC2 videoSpare;                                     // unused at present - set to 0
    SNC_UC4 audioSampleRate;                                // sample rate of audio
    SNC_UC2 audioChannels;                                  // number of channels
    SNC_UC2 audioSampleSize;                                // size of samples (in bits)
    SNC_UC2 audioSpare;                                     // unused at present - set to 0
    SNC_UC2 spare1;                                         // unused at present - set to 0
} SNC_RECORD_AVMUX;

// A convenient version for use by code

typedef struct
{
    int avmuxSubtype;                                       // the mux subtype
    unsigned char videoSubtype;                             // type of video stream
    unsigned char audioSubtype;                             // type of audio stream
    int videoWidth;                                         // width of frame
    int videoHeight;                                        // height of frame
    int videoFramerate;                                     // framerate
    int audioSampleRate;                                    // sample rate of audio
    int audioChannels;                                      // number of channels
    int audioSampleSize;                                    // size of samples (in bits)
} SNC_AVPARAMS;


//  SNC_RECORD_VIDEO - used to send video frames
//
//  The structure follows the SNC_EHEAD structure. Immediately following this structure
//  is the image data.

typedef struct
{
    SNC_RECORD_HEADER recordHeader;                         // the record type header
    SNC_UC2 width;                                          // width of each image
    SNC_UC2 height;                                         // height of each image
    SNC_UC4 size;                                           // size of the image
} SNC_RECORD_VIDEO;

//  SNC_RECORD_AUDIO - used to send audio data
//
//  The structure follows the SNC_EHEAD structure. Immediately following this structure
//  is the audio data.

typedef struct
{
    SNC_RECORD_HEADER recordHeader;                         // the record type header
    SNC_UC4 size;                                           // size of the audio data
    SNC_UC4 bitrate;                                        // bitrate of audio
    SNC_UC4 sampleRate;                                     // sample rate of audio
    SNC_UC2 channels;                                       // number of channels
    SNC_UC2 sampleSize;                                     // size of samples (in bits)
} SNC_RECORD_AUDIO;


//  SNC_PTZ - used to send PTZ information
//
//  Data is sent in servo form, i.e. 1 (SNC_SERVO_LEFT) to 0xffff (SNC_SERVO_RIGHT).
//  SNC_SERVER_CENTER means center camera in that axis. For zoom, it menas some default position.

typedef struct
{
    SNC_UC2 pan;                                            // pan position
    SNC_UC2 tilt;                                           // tilt position
    SNC_UC2 zoom;                                           // zoom position
} SNC_PTZ;

#endif  // _SNCAVDEFS_H

