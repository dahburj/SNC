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

#ifndef _SNCDEFS_H
#define _SNCDEFS_H

#include <qglobal.h>

//  Version

#define SNCLIB_VERSION  "2.0.0"

//  Timer definition
//
//  SNC_CLOCKS_PER_SEC deifnes how many clock ticks are in one second

#define SNC_CLOCKS_PER_SEC      1000                        // 1000 ticks of mS clock equals one second

//-------------------------------------------------------------------------------------------
//	Reserved characters and their functions
//

#define	SNC_SERVICEPATH_SEP     '/'                         // the path component separator character
#define	SNC_STREAM_TYPE_SEP     ':'                         // used to delimit a stream type ina path
#define	SNCCFS_FILENAME_SEP     ';'                         // used to separate file paths in a directory string

//-------------------------------------------------------------------------------------------
//	The defines below are the most critical SNC system definitions.
//

//	SNC constants

#define SNC_HEARTBEAT_INTERVAL          5                   // default heartbeatinterval in seconds
#define SNC_HEARTBEAT_TIMEOUT           3                   // default number of missed heartbeats before timeout

#define SNC_SERVICE_LOOKUP_INTERVAL     (5 * SNC_CLOCKS_PER_SEC)    // 5 seconds interval between service lookup requests

#define SNC_SOCKET_LOCAL		        1661                // socket for the SNCControl
#define SNC_SOCKET_LOCAL_ENCRYPT        1662				// SSL socket for the SNCControl

#define SNC_PRIMARY_SOCKET_STATICTUNNEL	1806                // socket for primary static SNCControl tunnels
#define SNC_BACKUP_SOCKET_STATICTUNNEL	1807                // socket for backup static SNCControl tunnels

#define SNC_PRIMARY_SOCKET_STATICTUNNEL_ENCRYPT	1808        // SSL socket for primary static SNCControl tunnels
#define SNC_BACKUP_SOCKET_STATICTUNNEL_ENCRYPT	1809        // SSL socket for backup static SNCControl tunnels

#define SNC_MAX_CONNECTEDCOMPONENTS     2048                // maximum directly connected components to a component
#define SNC_MAX_CONNECTIONIDS           (SNC_MAX_CONNECTEDCOMPONENTS * 2) // used to uniquely identify sockets
#define SNC_MAX_COMPONENTSPERDEVICE     256                 // maximum number of components assigned to a single device
#define SNC_MAX_SERVICESPERCOMPONENT    128                 // max number of services in a component
                                                            // max possible multicast maps
#define SNC_MAX_DELENGTH                16384               // maximum possible directory entry length

//	SNC message size maximums

#define SNC_MESSAGE_MAX                 0x80000

//-------------------------------------------------------------------------------------------
//  IP related definitions

#define SNC_IPSTR_LEN                   16                  // size to use for IP string buffers (xxx.xxx.xxx.xxx plus 0)

#define SNC_IPADDR_LEN                  4                   // 4 byte IP address
typedef unsigned char SNC_IPADDR[SNC_IPADDR_LEN];           // a convenient type for IP addresses

//-------------------------------------------------------------------------------------------
//  Some general purpose typedefs - used esepcially for transferring values greater than
//  8 bits across the network.
//

typedef unsigned char SNC_UC2[2];                           // an array of two unsigned chars (short)
typedef unsigned char SNC_UC4[4];                           // an array of four unsigned chars (int)
typedef unsigned char SNC_UC8[8];                           // an array of eight unsigned chars (int64)

//-------------------------------------------------------------------------------------------
//  Some useful defines for MAC addresses
//

#define SNC_MACADDR_LEN	6                                   // length of a MAC address

typedef unsigned char SNC_MACADDR[SNC_MACADDR_LEN];         // a convenient type for MAC addresses
#define SNC_MACADDRSTR_LEN		(SNC_MACADDR_LEN*2+1)       // the hex string version
typedef char SNC_MACADDRSTR[SNC_MACADDRSTR_LEN];

//-------------------------------------------------------------------------------------------
//  The SNC UID (Unique Identifier) consists of a 6 byte MAC address (or equivalent)
//  along with a two byte instance number to ensure uniqueness on a device.
//

typedef struct
{
    SNC_MACADDR macAddr;                                    // the MAC address
    SNC_UC2 instance;                                       // the instance number
} SNC_UID;

// These defs are for a string version of the SNC_UID

#define SNC_UIDSTR_LEN	(sizeof(SNC_UID) * 2 + 1)           // length of string version as hex pairs plus 0
typedef char SNC_UIDSTR[SNC_UIDSTR_LEN];                    // and for the string version

//-------------------------------------------------------------------------------------------
//  Service Path Syntax
//
//  When a component wishes to communicate with a service on another component, it needs to locate it using
//  a service path string. This is mapped by SNCControl into a UID and port number.
//  The service string can take various forms (note that case is important):
//
//  Form 1 -    Service Name. In this case, the service path is simply the service name. SNCControl will
//              look up the name and stop at the first service with a matching name. for example, if the
//              service name is "video", the service path would be "video".
//
//  Form 2 -    Component name and service name. The service path is the component name, then "/" and
//              then the service name. SNCControl will only match the service name against
//              the component with the specified component name. For example, if the service name is "video"
//              and the component name is "WebCam", the service path would be "WebCam/video". "*" is a wildcard
//              so that "*/video" is equivalent to "video".
//
//  Form 3 -    Region name, component name and service name. The service path consists of a region name, then
//              a "/", then the cmoponent name, then a "/" and then the sevice name. As an example, if the
//              region name is "Robot1", the component name "WebCam" and the service name "video", the
//              service path would be "Robot1/WebCam/video". Again, "*" is a wildcard for regions and
//              components so that "Robot1/*/video" would match the first service called "video" found in region
//              "Robot1".

//  Config and Directory related definitions
//

#define SNC_MAX_TAG                     256                 // maximum length of tag string (including 0 at end)
#define SNC_MAX_NONTAG                  1024                // max length of value (including 0 at end)

#define SNC_MAX_NAME                    32                  // general max name length

#define SNC_MAX_APPNAME                 SNC_MAX_NAME        // max length of a zero-terminated app name
#define SNC_MAX_APPTYPE                 SNC_MAX_NAME        // max length of a zero-terminated app type
#define SNC_MAX_COMPTYPE                SNC_MAX_NAME        // max length of a zero-terminated component type
#define SNC_MAX_SERVNAME                SNC_MAX_NAME        // max length of a service name
#define SNC_MAX_REGIONNAME              SNC_MAX_NAME        // max length of a region name
#define SNC_MAX_SERVPATH                128                 // this is max size of the NULL terminated path for service paths

typedef char SNC_SERVNAME[SNC_MAX_SERVNAME];                // the service type
typedef char SNC_REGIONNAME[SNC_MAX_REGIONNAME];            // the region name type
typedef char SNC_APPNAME[SNC_MAX_APPNAME];                  // the app name
typedef char SNC_APPTYPE[SNC_MAX_APPNAME];                  // the app type
typedef char SNC_COMPTYPE[SNC_MAX_COMPTYPE];                // the component type type

typedef char SNC_SERVPATH[SNC_MAX_SERVPATH];                // the service path type

//-------------------------------------------------------------------------------------------
//  SNCCore App and Component Type defs

#define APPTYPE_CONTROL                 "SNCControl"
#define APPTYPE_EXEC                    "SNCExec"

#define COMPTYPE_CONTROL                "Control"
#define COMPTYPE_EXEC                   "Exec"
#define COMPTYPE_DB                     "DB"

//-------------------------------------------------------------------------------------------
//  SNCControl Directory Entry Tag Defs
//
//  A directory record as transmitted betweeen components and SNCControls looks like:
//
//<CMP>
//<UID>xxxxxxxxxxxxxxxxxx</UID>
//<NAM>name</NAME>                                          // component name
//<TYP>type</NAME>                                          // component type
//...
//</CMP>
//<CMP>
//...
//</CMP>
//...


//  These tags are the component container tags

#define DETAG_COMP                      "CMP"
#define DETAG_COMP_END                  "/CMP"

//  Component Directory Entry Tag Defs

#define DETAG_APPNAME                   "NAM"               // the app name
#define DETAG_COMPTYPE                  "TYP"               // a string identifying the component
#define DETAG_UID                       "UID"               // the UID
#define DETAG_MSERVICE                  "MSV"               // a string identifying a multicast service
#define DETAG_ESERVICE                  "ESV"               // a string identifying an E2E service
#define DETAG_NOSERVICE                 "NSV"               // a string identifying an empty service slot

//  E2E service code standard endpoint service names

#define DE_E2ESERVICE_PARAMS            "Params"            // parameter deployment service
#define DE_E2ESERVICE_SNCCFS            "SNCCFS"            // SNCCFS service

//	Service type codes

#define SERVICETYPE_MULTICAST           0                   // a multicast service
#define SERVICETYPE_E2E                 1                   // an end to end service
#define SERVICETYPE_NOSERVICE           2                   // a code indicating no service

//-------------------------------------------------------------------------------------------
//  SNC message types
//

//  HEARTBEAT
//  This message which is just the SNCMSG itself is sent regular by both parties
//  in a SNCLink. It's used to ensure correct operation of the link and to allow
//  the link to be re-setup if necessary. The message itself is a SNCHello data structure -
//  the same as is sent on the SNCHello system.
//
//  The HELLO structure that forms the message may also be followed by a properly
//  formatted directory entry (DE) as described above. If there is nothing present,
//  this means that DE for the component hasn't changed. Otherwise, the DE is used by the
//  receiving SNCControl as the new DE for the component.

#define SNCMSG_HEARTBEAT                1

//  DE_UPDATE
//  This message is sent by a Component to the SNCControl in order to transfer a DE.
//  Note that the message is sent as a null terminated string.

#define SNCMSG_SERVICE_LOOKUP_REQUEST   2

//  SERVICE_LOOKUP_RESPONSE
//  This message is sent back to a component with the results of the lookup.
//  The relevant fields are filled in the SNC_SERVICE_LOOKUP structure.

#define SNCMSG_SERVICE_LOOKUP_RESPONSE  3

//	DIRECTORY_REQUEST
//	An appliation can request a copy of the directory using this message.
//	There are no parameters or data - the message is just a SNC_MESSAGE

#define SNCMSG_DIRECTORY_REQUEST        4

//  DIRECTORY_RESPONSE
//  This message is sent to an application in response to a request.
//  The message consists of a SNC_DIRECTORY_RESPONSE structure.

#define SNCMSG_DIRECTORY_RESPONSE       5

//  SERVICE_ACTIVATE
//  This message is sent by a SNCControl to an SNCEndpoint multicast service when the SNCEndpoint
//  multicast should start generating data as someone has subscribed to its service.

#define SNCMSG_SERVICE_ACTIVATE         6

//  MULTICAST_FRAME
//  Multicast frames are sent using this message. The data is the parameter

#define SNCMSG_MULTICAST_MESSAGE        16

//  MULTICAST_ACK
//  This message is sent to acknowledge a multicast and request the next

#define	SNCMSG_MULTICAST_ACK            17

//  E2E - SNCEndpoint to SNCEndpoint message

#define SNCMSG_E2E                      18

#define SNCMSG_MAX                      18                  // highest legal message value

//-------------------------------------------------------------------------------------------
//  SNC_MESSAGE - the structure that defines the object transferred across
//  the SNCLink - the component to component header. This structure must
//  be the first entry in every message header.
//

//  The structure itself

typedef struct
{
    unsigned char cmd;                                      // the type of message
    SNC_UC4 len;                                            // message length (includes the SNC_MESSAGE itself and everything that follows)
    unsigned char flags;                                    // used to send priority
    unsigned char spare;                                    // to put on 32 bit boundary
    unsigned char cksm;                                     // checksum = 256 - sum of previous bytes as chars
} SNC_MESSAGE;

//  SNC_EHEAD - SNCEndpoint header
//
//  This is used to send messages between specific services within components.
//  seq is used to control the acknowledgement window. It starts off at zero
//  and increments with each new message. Acknowledgements indicate the next acceptable send
//  seq and so open the window again.

#define SNC_MAX_WINDOW	4                                   // the maximum number of outstanding messages

typedef struct
{
    SNC_MESSAGE SNCMessage;                                 // the SNCLink header
    SNC_UID sourceUID;                                      // source component of the endpoint message
    SNC_UID destUID;                                        // dest component of the message
    SNC_UC2 sourcePort;                                     // the source port number
    SNC_UC2 destPort;                                       // the destination port number
    unsigned char seq;                                      // seq number of message
    unsigned char par0;                                     // an application-specific parameter (for E2E use)
    unsigned char par1;
    unsigned char par2;                                     // make a multiple of 32 bits
} SNC_EHEAD;

//-------------------------------------------------------------------------------------------
//  The SNC_SERVICE_LOOKUP structure

#define SERVICE_LOOKUP_INTERVAL         (SNC_CLOCKS_PER_SEC * 2)    // while waiting
#define SERVICE_REFRESH_INTERVAL        (SNC_CLOCKS_PER_SEC * 4)    // when registered
#define SERVICE_REFRESH_TIMEOUT         (SERVICE_REFRESH_INTERVAL * 3) // Refresh timeout period
#define SERVICE_REMOVING_INTERVAL       (SNC_CLOCKS_PER_SEC * 4) // when closing
#define SERVICE_REMOVING_MAX_RETRIES    2                   // number of times an endpoint retries closing a remote service

#define SERVICE_LOOKUP_FAIL             0                   // not found
#define SERVICE_LOOKUP_SUCCEED          1                   // found and response fields filled in
#define SERVICE_LOOKUP_REMOVE           2                   // this is used to remove a multicast registration

typedef struct
{
    SNC_MESSAGE SNCMessage;                                 // the SNCLink header
    SNC_UID lookupUID;                                      // the returned UID of the service
    SNC_UC4 ID;                                             // the returned ID for this entry (to detect restarts requiring re-regs)
    SNC_SERVPATH servicePath;                               // the service path string to be looked up
    SNC_UC2 remotePort;                                     // the returned port to use for the service - the remote index for the service
    SNC_UC2 componentIndex;                                 // the returned component index on SNCControl (for fast refreshes)
    SNC_UC2 localPort;                                      // the port number of the requestor - the local index for the service
    unsigned char serviceType;                              // the service type requested
    unsigned char response;                                 // the response code
} SNC_SERVICE_LOOKUP;

typedef struct
{
    SNC_MESSAGE SNCMessage;                                 // the message header
    SNC_UC2 endpointPort;                                   // the endpoint service port to which this is directed
    SNC_UC2 componentIndex;                                 // the returned component index on SNCControl (for fast refreshes)
    SNC_UC2 SNCControlPort;                                 // the SNCControl port to send the messages to
    unsigned char response;                                 // the response code
} SNC_SERVICE_ACTIVATE;


//  SNCMESSAGE nFlags masks

#define SNCLINK_PRI                     0x03                // bits 0 and 1 are priority bits

#define SNCLINK_PRIORITIES              4                   // four priority levels

#define SNCLINK_HIGHPRI                 0                   // highest priority - typically for real time control data
#define SNCLINK_MEDHIGHPRI              1
#define SNCLINK_MEDPRI                  2
#define SNCLINK_LOWPRI                  3                   // lowest priority - typically for multicast information

//-------------------------------------------------------------------------------------------
//  SNC_DIRECTORY_RESPONSE - the directory response message structure
//

typedef struct
{
    SNC_MESSAGE SNCMessage;                                 // the message header
                                                            // the directory string follows
} SNC_DIRECTORY_RESPONSE;

//  Standard multicast stream names

#define SNC_STREAMNAME_AVMUX            "avmux"
#define	SNC_STREAMNAME_AVMUXLR          "avmux:lr"
#define	SNC_STREAMNAME_AVMUXRAW         "avmux:raw"
#define SNC_STREAMNAME_SENSOR           "sensor"
#define SNC_STREAMNAME_HOMEAUTOMATION   "ha"
#define SNC_STREAMNAME_IMAGE            "image"
#define SNC_STREAMNAME_SENSORSTATS      "sensor_stats"
#define SNC_STREAMNAME_PACKETCAPTURE    "packet_capture"

//  Standard E2E stream names

#define SNC_STREAMNAME_MANAGE           "manage"
#define SNC_STREAMNAME_CONTROL          "control"
#define	SNC_STREAMNAME_CFS              "cfs"
#define	SNC_STREAMNAME_AWAREDB          "awareDB"


//-------------------------------------------------------------------------------------------
//  SNC Record Headers
//
//  Record headers must always have the record type as the first field.
//

typedef struct
{
    SNC_UC2 type;                                           // the record type code
    SNC_UC2 subType;                                        // the sub type code
    SNC_UC2 headerLength;                                   // total length of specific record header
    SNC_UC2 param;                                          // type specific parameter
    SNC_UC2 param1;                                         // another one
    SNC_UC2 param2;                                         // and another one
    SNC_UC4 recordIndex;                                    // a monotonically incerasing index number
    SNC_UC8 timestamp;                                      // timestamp for the sample
} SNC_RECORD_HEADER;

//  Major type codes

#define SNC_RECORD_TYPE_VIDEO           0                   // a video record
#define SNC_RECORD_TYPE_AUDIO           1                   // an audio record
#define SNC_RECORD_TYPE_SENSOR          4                   // multiplexed sensor data
#define SNC_RECORD_TYPE_AVMUX           12                  // an avmux stream record
#define SNC_RECORD_TYPE_IMAGE           13                  // an image stream record
#define SNC_RECORD_TYPE_SENSORSTATS     14                  // a sensor stats record

#define SNC_RECORD_TYPE_JSON            32                  // indicates a JSON format record

#define SNC_RECORD_TYPE_USER            (0x8000)            // user defined codes start here

//  Store format options

#define SNC_RECORD_STORE_FORMAT_SRF     "srf"               // structured record file
#define SNC_RECORD_STORE_FORMAT_RAW     "raw"               // raw file format (just data - no record headers on any sort)

//  Flat file defs

#define SNC_RECORD_FLAT_EXT             "dat"
#define SNC_RECORD_FLAT_FILTER          "*.dat"

//  SRF defs

#define SYNC_LENGTH                     8                   // 8 bytes in sync sequence

#define SYNC_STRINGV0                   "SpRSHdV0"          // for version 0

#define SNC_RECORD_SRF_RECORD_EXT       "srf"               // file extension for record files
#define SNC_RECORD_SRF_INDEX_EXT        "srx"               // file extension for index files
#define SNC_RECORD_SRF_RECORD_DOTEXT    ".srf"              // file extension for record files with .
#define SNC_RECORD_SRF_INDEX_DOTEXT     ".srx"              // file extension for index files with .
#define SNC_RECORD_SRF_RECORD_FILTER    "*.srf"             // filter for record files
#define SNC_RECORD_SRF_INDEX_FILTER     "*.srx"             // filter extension for index files

//  The record header that's stored with a record in an srf file

typedef struct
{
    char sync[SYNC_LENGTH];                                 // the sync sequence
    SNC_UC4 size;                                           // size of the record that follows
    SNC_UC4 data;                                           // unused at this time
} SNC_STORE_RECORD_HEADER;

//  SNC packet capture defs

typedef struct
{
    quint8 dest[SNC_MACADDR_LEN];
    quint8 src[SNC_MACADDR_LEN];
    quint16 etherType;

} SNC_MAC_HEADER;

typedef struct
{
    quint8 verLen;
    quint8 dscpEcn;
    quint8 totalLength[2];
    quint16 ident;
    quint16 flagsFragOff;
    quint8 ttl;
    quint8 protocol;
    quint16 checksum;
    quint8 srcAddr[4];
    quint8 dstAddr[4];
} SNC_IP_HEADER;


#endif // _SNCDEFS_H

