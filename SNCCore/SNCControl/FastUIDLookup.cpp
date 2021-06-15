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

#include "FastUIDLookup.h"

#include "SNCUtils.h"

FastUIDLookup::FastUIDLookup(void)
{
    int		i;

    for (i = 0; i < FUL_LEVEL_SIZE; i++)
        FULLevel0[i] = NULL;
}

FastUIDLookup::~FastUIDLookup(void)
{
    int indexL0, indexL1, indexL2;
    void **L1, **L2, **L3;

    for (indexL0 = 0; indexL0 < FUL_LEVEL_SIZE; indexL0++) {
        if (FULLevel0[indexL0] != NULL) {
            L1 = (void **)FULLevel0[indexL0];
            for (indexL1 = 0; indexL1 < FUL_LEVEL_SIZE; indexL1++) {
                if (L1[indexL1] != NULL) {
                    L2 = (void **)L1[indexL1];
                    for (indexL2 = 0; indexL2 < FUL_LEVEL_SIZE; indexL2++) {
                        if (L2[indexL2] != NULL) {
                            L3 = (void **)L2[indexL2];
                            free(L3);
                        }
                    }
                    free(L2);
                }
            }
            free(L1);
        }
    }
}

void *FastUIDLookup::FULLookup(SNC_UID *UID)
{
    unsigned int indexL0, indexL1, indexL2, indexL3;
    void **ptr;
    SNC_UC2 *U2;
    void *returnValue;

    U2 = (SNC_UC2 *)UID;
    indexL0 = SNCUtils::convertUC2ToUInt(U2[0]);						// get UID as four separate unsigned ints
    indexL1 = SNCUtils::convertUC2ToUInt(U2[1]);
    indexL2 = SNCUtils::convertUC2ToUInt(U2[2]);
    indexL3 = SNCUtils::convertUC2ToUInt(U2[3]);

    QMutexLocker locker (&m_lock);

//	Do level 0 lookup

    ptr = (void **)FULLevel0[indexL0];
    if (ptr == NULL)
        return NULL;

//	Do level 1 lookup

    ptr = (void **)ptr[indexL1];
    if (ptr == NULL)
        return NULL;

//	Do level 2 lookup

    ptr = (void **)ptr[indexL2];
    if (ptr == NULL)
        return NULL;

//	Do level 3 lookup

    returnValue =  ptr[indexL3];
    return returnValue;
}

void	FastUIDLookup::FULAdd(SNC_UID *UID, void *data)
{
    unsigned int indexL0, indexL1, indexL2, indexL3;
    void **L1, **L2, **L3, *ptr;
    SNC_UC2 *U2;

    U2 = (SNC_UC2 *)UID;
    indexL0 = SNCUtils::convertUC2ToUInt(U2[0]);						// get UID as four separate unsigned ints
    indexL1 = SNCUtils::convertUC2ToUInt(U2[1]);
    indexL2 = SNCUtils::convertUC2ToUInt(U2[2]);
    indexL3 = SNCUtils::convertUC2ToUInt(U2[3]);

    if ((ptr = FULLookup(UID)) != NULL)
        FULDelete(UID);

    QMutexLocker locker(&m_lock);

    if ((L1 = (void **)FULLevel0[indexL0]) == NULL)
    {														// need to add a level 1 array
        FULLevel0[indexL0] = (void *)calloc(FUL_LEVEL_SIZE, sizeof (void *));
        L1 = (void **)FULLevel0[indexL0];
    }
    if ((L2 = (void **)L1[indexL1]) == NULL)
    {														// need to add a level 2 array
        L1[indexL1] = (void *)calloc(FUL_LEVEL_SIZE, sizeof(void *));
        L2 = (void **)L1[indexL1];
    }
    if ((L3 = (void **)L2[indexL2]) == NULL)
    {														// need to add a level 2 array
        L2[indexL2] = (void *)calloc(FUL_LEVEL_SIZE, sizeof(void *));
        L3 = (void **)L2[indexL2];
    }
    L3[indexL3] = data;										// finally, record the data
}

void FastUIDLookup::FULDelete(SNC_UID *UID)
{
    unsigned int indexL0, indexL1, indexL2, indexL3;
    void **L1, **L2, **L3;

    SNC_UC2 *U2;

    U2 = (SNC_UC2 *)UID;
    indexL0 = SNCUtils::convertUC2ToUInt(U2[0]);	// get UID as four separate unsigned ints
    indexL1 = SNCUtils::convertUC2ToUInt(U2[1]);
    indexL2 = SNCUtils::convertUC2ToUInt(U2[2]);
    indexL3 = SNCUtils::convertUC2ToUInt(U2[3]);

    QMutexLocker locker(&m_lock);

//	Do level 0 lookup

    L1 = (void **)FULLevel0[indexL0];
    if (L1 == NULL)
        return;
//	Do level 1 lookup

    L2 = (void **)L1[indexL1];
    if (L2 == NULL)
        return;

//	Do level 2 lookup

    L3 = (void **)L2[indexL2];
    if (L3 == NULL)
        return;

//	Clear data pointer

    L3[indexL3] = NULL;
}

