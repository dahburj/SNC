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

#ifndef _SNCCFSDEFS_H
#define _SNCCFSDEFS_H

//  SNC_CFSHEADER is used on every SNCCFS E2E message
//  cfsType is used to hold the message type code.
//  cfsParam depends on the cfsType field. If it is a response, this is a response code.
//  cfsIndex is the index of the record referred to in the req/res. Not always used.
//  cfsLength is the total length of data that follows the header. The type of data depends on cfsType.

//  Note about file handles and response codes
//
//  Since file handles and response codes can share the same field in some cases,
//  the top bit (bit 15) of the field is used to indicate which it is. Error response codes
//  have the top bit set. So, if the top bit is clear, this is a success response and the remaining
//  15 bits may contain the stream handle (on an open response). If the top bit is set, it is an error
//  response code and the remaining 15 bits indicate the type.

#include "SNCDefs.h"
typedef struct
{
    SNC_UC2 cfsType;                                        // the message type
    SNC_UC2 cfsParam;                                       // a parameter value used for response codes for example
    SNC_UC2 cfsStoreHandle;                                 // the handle used to identify an open file at the SNCCFS store
    SNC_UC2 cfsClientHandle;                                // the handle used to identify an open file at the client
    SNC_UC4 cfsIndex;                                       // record index or timestamp
    SNC_UC4 cfsLength;                                      // length of data that follows this structure (record, stream name etc)
} SNC_CFSHEADER;

//  Index file entries

typedef struct
{
    SNC_UC8 cfsPointer;                                     // pointer into record file
    SNC_UC8 cfsTimestamp;                                   // timestamp of record
} SNC_CFSINDEX;

//  SNCCFS message type codes
//
//  Note: cfsLength is alsways used and must be set to zero if the message is just the SNC_CFSHEADER

//  SNCCFS_TYPE_DIR_REQ is sent to a SNCCFS store to request a list of available files.
//  cfsParam, cfsStoreHandle, cfsClientHandle, cfsIndex are not used.

#define SNCCFS_TYPE_DIR_REQ             0                   // file directory request

//  SNCCFS_TYPE_DIR_RES is returned from a SNCCFS store in response to a request.
//  cfsParam contains a response code. If it indicates success, the file names
//  follow the header as an XML record as a zero terminated string.
//  cfsLength indicates the total length of the string including the terminating zero.
//  cfsIndex cfsStoreHandle, cfsClientHandle is not used.

#define SNCCFS_TYPE_DIR_RES             1                   // file directory response

//  SNCCFS_TYPE_OPEN_REQ is sent to a SNCCFS to open a file.
//  cfsParam is used to hold the requested block size for flat files (up to 65535 bytes) or 0 for structured.
//  cfsClientHandle contains the handle to be used by the client for this stream.
//  cfsStoreHandle is unused.
//  cfsIndex is not used.
//  The file path is a complete path relative to the store root (including subdirs and extension)
//  that follows the header. The total length of the file path (including the null) is in
//  cfsLength.

#define SNCCFS_TYPE_OPEN_REQ            2                   // file open request

//  SNCCFS_TYPE_OPEN_RES is sent from the SNCCFS in response to a request
//  cfsParam indicates the response code.
//  cfsClientHandle contains the handle to be used by the client for this file.
//  cfsStoreHandle contains the handle to be used by the store for this file.
//  cfsIndex is the number of records or blocks in the file.

#define SNCCFS_TYPE_OPEN_RES            3                   // file open response

//  SNCCFS_TYPE_CLOSE_REQ is sent to the SNCCFS to close an open file
//  cfsParam is unused.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex is unused.

//  cfsIndex and cfsLength are not used.

#define SNCCFS_TYPE_CLOSE_REQ           4                   // file close request

//  SNCCFS_TYPE_CLOSE_RES is send from the SNCCFS in response to a close request
//  cfsParam contains the response code. If successful, this is the file handle that was closed
//  and no longer valid. If unsuccessful, this is the error code.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex is unused.

#define SNCCFS_TYPE_CLOSE_RES           5                   // file close response

//  SNCCFS_TYPE_KEEPALIVE_REQ is sent to the SNCCFS to keep an open file alive
//  cfsParam contains the file handle.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex is not used.

#define SNCCFS_TYPE_KEEPALIVE_REQ       6                   // keep alive heartbeat request

//  SNCCFS_TYPE_KEEPALIVE_RES is sent from the SNCCFS in response to a request
//  cfsParam contains the stream handle for an open file or an error code.
//  cfsClientHandle contains the handle assigned to this stream.
//  cfsStoreHandle contains the handle assigned to this stream.
//  cfsIndex is not used.

#define SNCCFS_TYPE_KEEPALIVE_RES       6                   // keep alive heartbeat response

//  SNCCFS_TYPE_READ_INDEX_REQ is sent to the SNCCFS to request the record or block(s) at the index specified
//  cfsParam contains the number of blocks to be read.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex contains the record index to be read (structured) or the first block number to be read (flat).


#define	SNCCFS_TYPE_READ_INDEX_REQ      16                  // requests a read of record or block starting at index n

//  SNCCFS_TYPE_READ_INDEX_RES is send from the SNCCFS in response to a request.
//  cfsParam contains the file handle if successful, an error code otherwise.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex contains the record index (structured) or the first block number (flat file).
//  cfsLength indicates the total length of the record or block(s) that follows the header

#define SNCCFS_TYPE_READ_INDEX_RES      17                  // response to a read at index n - contains record or error code

//  SNCCFS_TYPE_WRITE_INDEX_REQ is sent to the SNCCFS to write the record or block(s) at the appropriate place
//  cfsParam contains the number of blocks to be written (for flat files, ignored for structured).
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex is 0 for reset file length to 0 before write. ANy other value means append.
//  cfsLength indicates the total length of the record or block(s) that follows the header.

#define SNCCFS_TYPE_WRITE_INDEX_REQ     18                  // requests a write of record or block(s) at end of file

//  SNCCFS_TYPE_WRITE_INDEX_RES is send from the SNCCFS in response to a request.
//  cfsParam contains the stream handle if successful, an error code otherwise.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsIndex contains the original index from the request.
//  cfsLength is unused.

#define SNCCFS_TYPE_WRITE_INDEX_RES     19                  // response to a write at index n - contains success or error code

//  SNCCFS_TYPE_READ_INDEX_INTERVAL_REQ is sent to the SNCCFS to request the recordat the index specified
//  cfsParam contains the time interval requested.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsTime is the timestamp requested. The record(s) with the nearest greater time will be returned.

#define	SNCCFS_TYPE_READ_TIME_INTERVAL_REQ   20             // requests a read of record or block starting at time n

//  SNCCFS_TYPE_READ_TIME_INTERVAL_RES is send from the SNCCFS in response to a request.
//  cfsParam contains the file handle if successful, an error code otherwise.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsTime contains the record time requested.
//  cfsLength indicates the total length of the record following the header

#define SNCCFS_TYPE_READ_TIME_INTERVAL_RES 21               // response to a read at index n - contains record or error code

//  SNCCFS_TYPE_READ_INDEX_COUNT_REQ is sent to the SNCCFS to request the recordat the index specified
//  cfsParam contains the records requested.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsTime is the timestamp requested. The record(s) with the nearest greater time will be returned.

#define	SNCCFS_TYPE_READ_TIME_COUNT_REQ   22                // requests a read of record or block starting at time n

//  SNCCFS_TYPE_READ_TIME_COUNT_RES is send from the SNCCFS in response to a request.
//  cfsParam contains the file handle if successful, an error code otherwise.
//  cfsClientHandle contains the handle assigned to this file.
//  cfsStoreHandle contains the handle assigned to this file.
//  cfsTime contains the record time requested.
//  cfsLength indicates the total length of the record following the header

#define SNCCFS_TYPE_READ_TIME_COUNT_RES   23                // response to a read at index n - contains record or error code

//  SNCCFS_TYPE_DATAGRAM is a simple means for sending custom messages
//  between client and server. The CFS header must be present but only
//  the type and length fields are used.

#define SNCCFS_TYPE_DATAGRAM            24


//  SNCCFS Size Defines

#define SNCCFS_MAX_CLIENT_FILES         32                  // max files a client can have open at one time per EP

//  SNCCFS Error Response codes

#define SNCCFS_SUCCESS                      0                           // this is a success code as top bit is zero
#define SNCCFS_ERROR_CODE                   0x8000                      // this is where error codes start

#define SNCCFS_ERROR_SERVICE_UNAVAILABLE    (SNCCFS_ERROR_CODE + 0)     // service endpoint is not available
#define SNCCFS_ERROR_REQUEST_ACTIVE         (SNCCFS_ERROR_CODE + 1)     // service request already active (usually directory)
#define SNCCFS_ERROR_REQUEST_TIMEOUT        (SNCCFS_ERROR_CODE + 2)     // service request timeout
#define SNCCFS_ERROR_UNRECOGNIZED_COMMAND   (SNCCFS_ERROR_CODE + 3)     // cfsType wasn't recognized
#define SNCCFS_ERROR_MAX_CLIENT_FILES       (SNCCFS_ERROR_CODE + 4)     // too many files open at client
#define SNCCFS_ERROR_MAX_STORE_FILES        (SNCCFS_ERROR_CODE + 5)     // too many files open at store
#define SNCCFS_ERROR_FILE_NOT_FOUND         (SNCCFS_ERROR_CODE + 6)     // file path wasn't in an open request
#define SNCCFS_ERROR_INDEX_FILE_NOT_FOUND   (SNCCFS_ERROR_CODE + 7)     // couldn't find index file
#define SNCCFS_ERROR_FILE_INVALID_FORMAT    (SNCCFS_ERROR_CODE + 8)     // file isn't a structured type
#define SNCCFS_ERROR_INVALID_HANDLE         (SNCCFS_ERROR_CODE + 9)     // file handle not valid
#define SNCCFS_ERROR_INVALID_RECORD_INDEX   (SNCCFS_ERROR_CODE + 10)    // record index beyond end of file
#define SNCCFS_ERROR_READING_INDEX_FILE     (SNCCFS_ERROR_CODE + 11)    // read of index file failed
#define SNCCFS_ERROR_RECORD_SEEK            (SNCCFS_ERROR_CODE + 12)    // record file seek failed
#define SNCCFS_ERROR_RECORD_READ            (SNCCFS_ERROR_CODE + 13)    // record file read failed
#define SNCCFS_ERROR_INVALID_HEADER         (SNCCFS_ERROR_CODE + 14)    // record header is invalid
#define SNCCFS_ERROR_INDEX_WRITE            (SNCCFS_ERROR_CODE + 15)    // index file write failed
#define SNCCFS_ERROR_WRITE                  (SNCCFS_ERROR_CODE + 16)    // flat file write failed
#define SNCCFS_ERROR_TRANSFER_TOO_LONG      (SNCCFS_ERROR_CODE + 17)    // if message would be too long
#define SNCCFS_ERROR_READ                   (SNCCFS_ERROR_CODE + 18)    // flat file read failed
#define SNCCFS_ERROR_BAD_BLOCKSIZE_REQUEST  (SNCCFS_ERROR_CODE + 19)    // flat file access cannot request a block size < zero
#define SNCCFS_ERROR_INVALID_REQUEST_TYPE   (SNCCFS_ERROR_CODE + 20)    // request type not valid for this cfs mode
#define SNCCFS_ERROR_WRITE_TOO_SHORT        (SNCCFS_ERROR_CODE + 21)    // if not enough bytes for SNC record header
#define SNCCFS_ERROR_INDEX_SEEK             (SNCCFS_ERROR_CODE + 22)    // if seek in index file failed

//  SNCCFS Timer Values

#define SNCCFS_DIRREQ_TIMEOUT           (SNC_CLOCKS_PER_SEC * 5)        // how long to wait for a directory response
#define SNCCFS_OPENREQ_TIMEOUT          (SNC_CLOCKS_PER_SEC * 5)        // how long to wait for a file open response
#define SNCCFS_READREQ_TIMEOUT          (SNC_CLOCKS_PER_SEC * 5)        // how long to wait for a file read response
#define SNCCFS_WRITEREQ_TIMEOUT         (SNC_CLOCKS_PER_SEC * 5)        // how long to wait for a file write response
#define SNCCFS_QUERYREQ_TIMEOUT         (SNC_CLOCKS_PER_SEC * 5)
#define SNCCFS_CLOSEREQ_TIMEOUT         (SNC_CLOCKS_PER_SEC * 5)        // how long to wait for a file close response
#define SNCCFS_KEEPALIVE_INTERVAL       (SNC_CLOCKS_PER_SEC * 5)        // interval between keep alives
#define SNCCFS_KEEPALIVE_TIMEOUT        (SNCCFS_KEEPALIVE_INTERVAL * 3) // at which point the file is considered closed

//  SNCCFS E2E Message Priority
//  Always use this priority for SNCCFS messages. Random selection could lead to re-ordering

#define SNCCFS_E2E_PRIORITY             (SNCLINK_MEDHIGHPRI)

// SNCCFS OPENREQ parameters for cfsType
#define SNCCFS_TYPE_STRUCTURED_FILE     0
#define SNCCFS_TYPE_RAW_FILE            1

#endif  // _SNCDEFS_H
