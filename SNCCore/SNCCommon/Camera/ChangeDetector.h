﻿////////////////////////////////////////////////////////////////////////////
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

// This is based in part on TonyJpegDecoder - original license information below:

/****************************************************************************
*   Author:         Dr. Tony Lin                                            *
*   Email:          lintong@cis.pku.edu.cn                                  *
*   Release Date:   Dec. 2002                                               *
*                                                                           *
*   Name:           TonyJpegLib, rewritten from IJG codes                   *
*   Source:         IJG v.6a JPEG LIB                                       *
*   Purpose       Support real jpeg file, with readable code                *
*                                                                           *
*   Acknowlegement: Thanks for great IJG, and Chris Losinger                *
*                                                                           *
*   Legal Issues:   (almost same as IJG with followings)                    *
*                                                                           *
*   1. We don't promise that this software works.                           *
*   2. You can use this software for whatever you want.                     *
*   You don't have to pay.                                                  *
*   3. You may not pretend that you wrote this software. If you use it      *
*   in a program, you must acknowledge somewhere. That is, please           *
*   metion IJG, and Me, Dr. Tony Lin.                                       *
*                                                                           *
*****************************************************************************/

#ifndef CHANGEDETECTOR_H
#define CHANGEDETECTOR_H

#include <qimage.h>

#define CHANGEDETECTOR_SAMPLES     4                        // only 4 samples from each 8 x 8 block recovered

typedef struct {
 int componentId;                                          // identifier for this component (0..255)
 int componentIndex;                                       // its index in SOF or cinfo->comp_info[]
 int hSampFactor;                                          // horizontal sampling factor (1..4)
 int vSampFactor;                                          // vertical sampling factor (1..4)
 int quantTblNo;                                           // quantization table selector (0..3)
} JPEG_COMPONENT_INFO;

// Derived data constructed for each Huffman table

typedef struct{
   int				mincode[17];                            // smallest code of length k
   int				maxcode[18];                            // largest code of length k (-1 if none)
   int				valptr[17];                             // huffval[] index of 1st symbol of length k
   unsigned char	bits[17];                               // bits[k] = # of symbols with codes of
   unsigned char	huffval[256];                           // The symbols, in order of incr code length
   int				look_nbits[256];                        // # bits, or 0 if too long
   unsigned char	look_sym[256];                          // symbol, or unused
} HUFFTABLE;

class ChangeDetector
{
public:

   ChangeDetector();
   ~ChangeDetector();

   void setNoiseThreshold(int threshold);
   void setDeltaThreshold(int threshold);
   void setUninitialized();
   void setTilesToSkip(int skipTiles);
   void setIntervalsToSkip(int skipIntervals);
   bool imageChanged(const QByteArray &newJpeg);

   QByteArray makeJpegDataImage();

private:
   bool readJpgHeader(unsigned char *jpeg,	int jpegLength,	int& width, int& height, int& headSize);
   bool decompressImage(unsigned char *jpegData);
   void computeHuffmanTable(HUFFTABLE * dtbl);
   bool decompressOneTile();
   void inverseDCT();

   int readMarkers(unsigned char *jpeg, int& width, int& height, int& headSize);
   int readOneMarker();
   void getSof();
   void getDht();
   void getSos();
   void skipMarker();
   void getDri();
   void readRestartMarker();

   void huffmanDecode(int blockIndex);
   int getCategory(HUFFTABLE* htbl);
   void skipToNextRestart();
   void fillBitBuffer();
   int getBits(int nbits);
   int specialDecode(HUFFTABLE* htbl, int minBits);
   int valueFromCategory(int cate, int offset);

   unsigned char imageConvert(short val);

   bool m_initialized;                                     // true if initialized so some things can be skipped

   short *m_oldDataImage;                                  // the previous image for change detection
   short *m_dataImage;                                     // where the data image is stored
   int m_dataImageSize;                                    // size of the frequency image
   short * m_dataImagePtr;                                 // current pointer in data image
   int m_chunkLength;                                      // length of one chunk in a tile
   int m_tileLength;                                       // length of one frequency space tile
   int m_tileCount;                                        // number of tiles in the image
   short m_coeffs[64];                                     // the recovered DCT coefficients in a block

   int m_noiseThreshold;                                   // noise threshold for one chunk to be counted
   int m_deltaThreshold;                                   // overall threshold

   int m_tilesToSkip;                                      // the number of tiles to skip in a restart interval
   int m_intervalsToSkip;                                  // the number of restart intervals to skip

   int m_tilesToProcess;                                   // calculated number of tiles to process per restart interval

   HUFFTABLE m_htblYDC;
   HUFFTABLE m_htblYAC;
   HUFFTABLE m_htblCbCrDC;
   HUFFTABLE m_htblCbCrAC;

   unsigned short m_width;
   unsigned short m_height;
   unsigned short m_hMcuSize;
   unsigned short m_vMcuSize;
   unsigned short m_blocksInMcu;
   unsigned short m_cxTile;
   unsigned short m_cyTile;

   int m_dcY;
   int m_dcCb;
   int m_dcCr;

   int m_getBits;
   int m_getBuff;
   int m_dataBytesLeft;

   unsigned char * m_data;

   int m_precision;
   int m_component;

   int m_restartInterval;
   int m_restartsToGo;
   int m_unreadMarker;
   int m_nextRestartNum;

   JPEG_COMPONENT_INFO m_compInfo[3];

   int m_intDiscard;
   unsigned char m_charDiscard;
};

#endif // CHANGEDETECTOR_H
