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

#ifndef FASTUIDLOOKUP_H
#define FASTUIDLOOKUP_H

#include "SNCDefs.h"

#include <qmutex.h>

class FastUIDLookup
{

public:

//  Construction

    FastUIDLookup(void);
    ~FastUIDLookup(void);

    //  Fast UID Lookup functions and variables

#define FUL_LEVEL_SIZE                  0x10000				// 16 bits of lookup per array

public:
    void *FULLookup(SNC_UID *UID);                          // looks up a UID and returns the data pointer, NULL if not found
    void FULAdd(SNC_UID *UID, void *data);                  // adds a UID to the fast lookup system
    void FULDelete(SNC_UID *UID);                           // deletes a UID from the fast lookup system

protected:
    void *FULLevel0[FUL_LEVEL_SIZE];                        // the level 0 array
    QMutex m_lock;                                          // to ensure consistency
};

#endif // FASTUIDLOOKUP_h

