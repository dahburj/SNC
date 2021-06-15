#!/usr/bin/python

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

# The MIT License (MIT)
#
# Copyright (c) 2017 Tony DiCola for Adafruit Industries
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

Some code from Adafruti driver:



'''

from RT_I2C import RT_I2C
from RT_NullSensor import RT_NullSensor

# Register definitions    

TSL2591_ADDR                = 0x29
TSL2591_COMMAND_BIT         = 0xA0
TSL2591_ENABLE_POWEROFF     = 0x00
TSL2591_ENABLE_POWERON      = 0x01
TSL2591_ENABLE_AEN          = 0x02
TSL2591_ENABLE_AIEN         = 0x10
TSL2591_ENABLE_NPIEN        = 0x80
TSL2591_REGISTER_ENABLE     = 0x00
TSL2591_REGISTER_CONTROL    = 0x01
TSL2591_REGISTER_DEVICE_ID  = 0x12
TSL2591_REGISTER_CHAN0_LOW  = 0x14
TSL2591_REGISTER_CHAN1_LOW  = 0x16
TSL2591_LUX_DF              = 408.0
TSL2591_LUX_COEFB           = 1.64
TSL2591_LUX_COEFC           = 0.59
TSL2591_LUX_COEFD           = 0.86
TSL2591_MAX_COUNT_100MS     = 36863 # 0x8FFF
TSL2591_MAX_COUNT           = 65535 # 0xFFFF

# User-facing constants:
GAIN_LOW                  = 0x00  # low gain (1x)
"""Low gain (1x)"""
GAIN_MED                  = 0x10  # medium gain (25x)
"""Medium gain (25x)"""
GAIN_HIGH                 = 0x20  # medium gain (428x)
"""High gain (428x)"""
GAIN_MAX                  = 0x30  # max gain (9876x)
"""Max gain (9876x)"""
INTEGRATIONTIME_100MS     = 0x00  # 100 millis
"""100 millis"""
INTEGRATIONTIME_200MS     = 0x01  # 200 millis
"""200 millis"""
INTEGRATIONTIME_300MS     = 0x02  # 300 millis
"""300 millis"""
INTEGRATIONTIME_400MS     = 0x03  # 400 millis
"""400 millis"""
INTEGRATIONTIME_500MS     = 0x04  # 500 millis
"""500 millis"""
INTEGRATIONTIME_600MS     = 0x05  # 600 millis
"""600 millis"""

class RT_TSL2591(RT_NullSensor):
    ''' richards-tech driver for the TSL2591 light sensor '''
    
    def __init__(self):
        RT_NullSensor.__init__(self)
        self.sensorValid = True
        self.addr = TSL2591_ADDR
        self._integration_time = 0
        self._gain = 0
     
    def enable(self, busnum=-1, debug=False):
        # check if already enabled
        if (self.dataValid):
            return

        try:
            # check for the presence of the light sensor
            self.light = RT_I2C(self.addr, busnum, debug)
#            print('Light ID: 0x{0:02X}'.format(self.light.readU8(TSL2591_REGISTER_DEVICE_ID)))
            # start the light sensor running
            # Set default gain and integration times.
            self.gain = GAIN_LOW
            self.integration_time = INTEGRATIONTIME_100MS
            # Put the device in a powered on state after initialization.
            self.light.write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_ENABLE, TSL2591_ENABLE_POWERON | \
                       TSL2591_ENABLE_AEN | TSL2591_ENABLE_AIEN | \
                       TSL2591_ENABLE_NPIEN)
            self.dataValid = True
        except:
            pass
       
    def setIntegrationTime(self, integrationTime):
        self.integrationTime = integrationTime

    # Read the sensor
    def readLight(self):
        if not self.dataValid:
            return 0
            
        return self.lux

    @property
    def gain(self):
        """Get and set the gain of the sensor.  Can be a value of:

        - ``GAIN_LOW`` (1x)
        - ``GAIN_MED`` (25x)
        - ``GAIN_HIGH`` (428x)
        - ``GAIN_MAX`` (9876x)
        """
        control = self.light.readU8(TSL2591_REGISTER_CONTROL)
        return control & 0b00110000

    @gain.setter
    def gain(self, val):
        assert val in (GAIN_LOW, GAIN_MED, GAIN_HIGH, GAIN_MAX)
        # Set appropriate gain value.
        control = self.light.readU8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL)
        control &= 0b11001111
        control |= val
        self.light.write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL, control)
        # Keep track of gain for future lux calculations.
        self._gain = val

    @property
    def integration_time(self):
        """Get and set the integration time of the sensor.  Can be a value of:

        - ``INTEGRATIONTIME_100MS`` (100 millis)
        - ``INTEGRATIONTIME_200MS`` (200 millis)
        - ``INTEGRATIONTIME_300MS`` (300 millis)
        - ``INTEGRATIONTIME_400MS`` (400 millis)
        - ``INTEGRATIONTIME_500MS`` (500 millis)
        - ``INTEGRATIONTIME_600MS`` (600 millis)
        """
        control = self.light.readU8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL)
        return control & 0b00000111

    @integration_time.setter
    def integration_time(self, val):
        assert 0 <= val <= 5
        # Set control bits appropriately.
        control = self.light.readU8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL)
        control &= 0b11111000
        control |= val
        self.light.write8(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CONTROL, control)
        # Keep track of integration time for future reading delay times.
        self._integration_time = val

    @property
    def raw_luminosity(self):
        """Read the raw luminosity from the sensor (both IR + visible and IR
        only channels) and return a 2-tuple of those values.  The first value
        is IR + visible luminosity (channel 0) and the second is the IR only
        (channel 1).  Both values are 16-bit unsigned numbers (0-65535).
        """
        # Read both the luminosity channels.
        channel_0 = self.light.readU16(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CHAN0_LOW)
        channel_1 = self.light.readU16(TSL2591_COMMAND_BIT | TSL2591_REGISTER_CHAN1_LOW)
        return (channel_0, channel_1)

    @property
    def full_spectrum(self):
        """Read the full spectrum (IR + visible) light and return its value
        as a 32-bit unsigned number.
        """
        channel_0, channel_1 = self.raw_luminosity
        return (channel_1 << 16) | channel_0

    @property
    def infrared(self):
        """Read the infrared light and return its value as a 16-bit unsigned number.
        """
        _, channel_1 = self.raw_luminosity
        return channel_1

    @property
    def visible(self):
        """Read the visible light and return its value as a 32-bit unsigned number.
        """
        channel_0, channel_1 = self.raw_luminosity
        full = (channel_1 << 16) | channel_0
        return full - channel_1

    @property
    def lux(self):
        """Read the sensor and calculate a lux value from both its infrared
        and visible light channels.
        """
        channel_0, channel_1 = self.raw_luminosity

        # Compute the atime in milliseconds
        atime = 100.0 * self._integration_time + 100.0

        # Set the maximum sensor counts based on the integration time (atime) setting
        if self._integration_time == INTEGRATIONTIME_100MS:
            max_counts = TSL2591_MAX_COUNT_100MS
        else:
            max_counts = TSL2591_MAX_COUNT

        # Handle overflow.
        if channel_0 >= max_counts or channel_1 >= max_counts:
            return -1
        # Calculate lux using same equation as Arduino library:
        #  https://github.com/adafruit/Adafruit_TSL2591_Library/blob/master/Adafruit_TSL2591.cpp
        again = 1.0
        if self._gain == GAIN_MED:
            again = 25.0
        elif self._gain == GAIN_HIGH:
            again = 428.0
        elif self._gain == GAIN_MAX:
            again = 9876.0
        cpl = (atime * again) / TSL2591_LUX_DF
        lux1 = (channel_0 - (TSL2591_LUX_COEFB * channel_1)) / cpl
        lux2 = ((TSL2591_LUX_COEFC * channel_0) - (TSL2591_LUX_COEFD * channel_1)) / cpl
        return max(lux1, lux2)
        


