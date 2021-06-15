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

# Based Adafruit_I2C (the original had no license header unfortunately)

import smbus
import subprocess

class RT_I2C:
    @staticmethod
    def getPiRevision():
        "Gets the version number of the Raspberry Pi board"
        # Courtesy quick2wire-python-api
        # https://github.com/quick2wire/quick2wire-python-api
        # Updated revision info from: http://elinux.org/RPi_HardwareHistory#Board_Revision_History
        try:
            with open('/proc/cpuinfo','r') as f:
                for line in f:
                    if line.startswith('Revision'):
                        return 1 if line.rstrip()[-1] in ['2','3'] else 2
        except:
            return 0

    @staticmethod
    def getPiI2CBusNumber():
        # Gets the I2C bus number /dev/i2c#
        # return 1 if RT_I2C.getPiRevision() > 1 else 0
        return 1

    def __init__(self, address, busnum=-1, debug=False):
        self.address = address
        # By default, the correct I2C bus is auto-detected using /proc/cpuinfo
        # Alternatively, you can hard-code the bus version below:
        # self.bus = smbus.SMBus(0); # Force I2C0 (early 256MB Pi's)
        # self.bus = smbus.SMBus(1); # Force I2C1 (512MB Pi's)
        self.bus = smbus.SMBus(busnum if busnum >= 0 else RT_I2C.getPiI2CBusNumber())
        self.debug = debug
        
    @staticmethod
    def checkI2CAddress(addr):
        # returns true if address occupied
        
        # get the i2cdetect output
        
        p = subprocess.Popen(['i2cdetect', '-y',str(RT_I2C.getPiI2CBusNumber())],stdout=subprocess.PIPE,) 
        p.stdout.readline()     # dump header line
        
        # calculate relevant line
        
        lineNo = addr // 16;
        
        for i in range(0,lineNo):
            line = str(p.stdout.readline())
            
        splitString = p.stdout.readline().decode('utf8').split(' ')
        return splitString[(addr % 16) + 1] != '--'

    def reverseByteOrder(self, data):
        "Reverses the byte order of an int (16-bit) or long (32-bit) value"
        # Courtesy Vishal Sapre
        byteCount = len(hex(data)[2:].replace('L','')[::2])
        val = 0
        for i in range(byteCount):
            val    = (val << 8) | (data & 0xff)
            data >>= 8
        return val

    def errMsg(self):
        print ("Error accessing 0x{0:02X}: Check your I2C address".format(self.address))
        return -1

    def write8(self, reg, value):
        "Writes an 8-bit value to the specified register/address"
        try:
            self.bus.write_byte_data(self.address, reg, value)
            if self.debug:
                print ("I2C: Wrote 0x{0:02X} to register 0x{1:02X}",format(value, reg))
        except (IOError, err) as e:
            return self.errMsg()

    def write16(self, reg, value):
        "Writes a 16-bit value to the specified register/address pair"
        try:
            self.bus.write_word_data(self.address, reg, value)
            if self.debug:
                print ("I2C: Wrote 0x{0:02X} to register pair 0x{1:02X}, 0x{2:02X}".format(value, reg, reg+1))
        except (IOError, err) as e:
            return self.errMsg()

    def writeRaw8(self, value):
        "Writes an 8-bit value on the bus"
        try:
            self.bus.write_byte(self.address, value)
            if self.debug:
                print ("I2C: Wrote 0x{0:02X}".format(value))
        except (IOError, err) as e:
            return self.errMsg()

    def writeList(self, reg, list):
        "Writes an array of bytes using I2C format"
        try:
            if self.debug:
                print("I2C: Writing list to register 0x{0:02X}:".format(reg))
                print (list)
            self.bus.write_i2c_block_data(self.address, reg, list)
        except (IOError, err) as e:
            return self.errMsg()

    def readList(self, reg, length):
        "Read a list of bytes from the I2C device"
        try:
            results = self.bus.read_i2c_block_data(self.address, reg, length)
            if self.debug:
                print ("I2C: Device 0x{0:02X} returned the following from reg 0x{1:02X}".format(self.address, reg))
                print (results)
            return results
        except (IOError, err) as e:
            return self.errMsg()

    def readU8(self, reg):
        "Read an unsigned byte from the I2C device"
        try:
            result = self.bus.read_byte_data(self.address, reg)
            if self.debug:
                print ("I2C: Device 0x{0:02X} returned 0x{1:02X} from reg 0x{2:02X}".format(self.address, result & 0xFF, reg))
            return result
        except (IOError, err) as e:
            return self.errMsg()

    def readU16(self, reg):
        "Read an unsigned word from the I2C device"
        try:
            result = self.bus.read_word_data(self.address, reg)
            if self.debug:
                print ("I2C: Device 0x{0:02X} returned 0x{1:04X} from reg 0x{2:02X}".format(self.address, result & 0xFFFF, reg))
            return result
        except (IOError, err) as e:
            return self.errMsg()

    def readS8(self, reg):
        "Reads a signed byte from the I2C device"
        try:
            result = self.bus.read_byte_data(self.address, reg)
            if result > 127: 
                result -= 256
            if self.debug:
                print ("I2C: Device 0x{0:02X} returned 0x{1:02X} from reg 0x{2:02X}".format(self.address, result & 0xFF, reg))
            return result
        except (IOError, err) as e:
            return self.errMsg()
            
            
    def readSpecial(self):
        # just does a read without specifying a register
        try:
            result = self.bus.read_byte(self.address)
            if self.debug:
                print ("I2C: Device (special read) 0x{0:02X} returned 0x{1:02X}".format(self.address, result & 0xFF, reg))
            return result
        except (IOError, err) as e:
            return self.errMsg()
        

