'''

  This file is part of SNC

  Copyright (c) 2014-2021, Richard Barnett

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
  Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

'''

# This file contains useful SyntroNet definitions

# Reserved characters and their functions

SNC_SERVICEPATH_SEP = '/'                                # the path component separator
SNC_STREAM_TYPE_SEP = ':'                                # used to delimit a stream type in path
SNCCFS_FILENAME_SEP = ';'                                # used to seperate file paths in a CFS dir string
SNC_LOG_COMPONENT_SEP = '|'                              # separates parts of log messages

#   Standard multicast stream names

SNC_STREAMNAME_AVMUX = "avmux"
SNC_STREAMNAME_AVMUXLR = "avmux:lr"
SNC_STREAMNAME_AVMUXMOBILE = "avmuxmobile"
SNC_STREAMNAME_VIDEO = "video"
SNC_STREAMNAME_VIDEOLR = "video:lr"
SNC_STREAMNAME_AUDIO = "audio"
SNC_STREAMNAME_NAV = "nav"
SNC_STREAMNAME_LOG = "log"
SNC_STREAMNAME_SENSOR = "sensor"
SNC_STREAMNAME_TEMPERATURE = "temperature"
SNC_STREAMNAME_HUMIDITY = "humidity"
SNC_STREAMNAME_LIGHT = "light"
SNC_STREAMNAME_MOTION = "motion"
SNC_STREAMNAME_AIRQUALITY = "airquality"
SNC_STREAMNAME_PRESSURE = "pressure"
SNC_STREAMNAME_ACCELEROMETER = "accelerometer"
SNC_STREAMNAME_ZIGBEE_MULTICAST = "zbmc"
SNC_STREAMNAME_HOMEAUTOMATION = "ha"
SNC_STREAMNAME_THUMBNAIL = "thumbnail"
SNC_STREAMNAME_IMAGE = "image"

#   Standard E2E stream names

SNC_STREAMNAME_ZIGBEE_E2E = "zbe2e"
SNC_STREAMNAME_CONTROL = "control"
SNC_STREAMNAME_FILETRANSFER = "filetransfer"
SNC_STREAMNAME_PTZ = "ptz"
SNC_STREAMNAME_CFS = "cfs"

