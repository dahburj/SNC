#////////////////////////////////////////////////////////////////////////////
#//
#//  This file is part of SNC
#//
#//  Copyright (c) 2014-2021, Richard Barnett
#//
#//  Permission is hereby granted, free of charge, to any person obtaining a copy of
#//  this software and associated documentation files (the "Software"), to deal in
#//  the Software without restriction, including without limitation the rights to use,
#//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
#//  Software, and to permit persons to whom the Software is furnished to do so,
#//  subject to the following conditions:
#//
#//  The above copyright notice and this permission notice shall be included in all
#//  copies or substantial portions of the Software.
#//
#//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
#//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
#//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
#//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


cache()

TEMPLATE = app
TARGET = SNCView

QT += core gui network widgets

CONFIG += debug_and_release

unix {
        macx {
                target.path = /Applications
                QT += multimedia
        } else {
                target.path = /usr/bin
                LIBS += -lasound
        }

        INSTALLS += target
}

DEFINES += QT_NETWORK_LIB

QMAKE_LFLAGS += -no-pie

Release:DESTDIR = release
Release:OBJECTS_DIR = release/.obj
Release:MOC_DIR = release/.moc
Release:RCC_DIR = release/.rcc
Release:UI_DIR = release/.ui

Debug:DESTDIR = debug
Debug:OBJECTS_DIR = debug/.obj
Debug:MOC_DIR = debug/.moc
Debug:RCC_DIR = debug/.rcc
Debug:UI_DIR = debug/.ui

INCLUDEPATH += ../../SNCCore/SNCCommon/AVMux

include(SNCView.pri)
include(../../SNCCore/SNCLib/SNCLib.pri)
include(../../SNCCore/SNCJSON/SNCJSON.pri)
include(../../SNCCore/SNCCommon/GUI/GUI.pri)
