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

#ifndef DIRECTORYMANAGER_H
#define DIRECTORYMANAGER_H

#include "MulticastManager.h"

//  This is the DM_SERVICE data structure. It captures the service path for each service that has been advertised.
//
//  It also includes the UID and sequence of the component so that this structure can be used by a client
//  to identify the host component for the service

typedef struct
{
    bool valid;                                             // if this service port number is in use
    char serviceName[SNC_MAX_SERVNAME];                     // the service's name
    int port;                                               // the service port number (i.e. the service's index in the component DE)
    int serviceType;                                        // the type of service
    MM_MMAP *multicastMap;                                  // the multicast map entry (not used for end to end services)
} DM_SERVICE;


//  This is the DM_COMPONENT data structure. It contains information relevant to a component's services

typedef struct _DM_COMPONENT
{
    int sequenceID;                                         // the received DE's sequence ID
    char *originalDE;                                       // the original DE (allows for a quick change determination)
    char *localDE;                                          // same but with multicast port numbers made into local ports
    bool seenInDE;                                          // used when processing a DE to see if this component has vanished
    char appName[SNC_MAX_APPNAME];                          // the app's name
    char componentType[SNC_MAX_COMPTYPE];                   // the component's type
    SNC_UID componentUID;                                   // the component's UID
    SNC_UIDSTR UIDStr;                                      // the string version
    DM_SERVICE services[SNC_MAX_SERVICESPERCOMPONENT];      // the actual service array
    int serviceCount;                                       // number of entries in array
    struct _DM_COMPONENT *next;
} DM_COMPONENT;

//  DM_CONNECTEDCOMPONENT - contains information about each directly connected component

typedef struct
{
    bool valid;                                             // true if a DE has been received
    int index;                                              // index in the array
    SNC_UID connectedComponentUID;                          // the connected component's UID
    char *DE;                                               // a copy of the complete DE for the connected component (may have multiple sub records)
    DM_COMPONENT *componentDE;                              // pointer to a list of component directory entries
    void *data;                                             // this is usually a pointer to the Servo component entry. It's used in the FulAdd call.
} DM_CONNECTEDCOMPONENT;

class SNCServer;

class DirectoryManager : public QObject
{
    Q_OBJECT
public:

//  Construction

    DirectoryManager(void);
    ~DirectoryManager(void);

    SNCServer *m_server;

//  DMShutdown - must be called before deleting object

    void DMShutdown();

//  DMProcessDE - processes a Directory Entry from a component
//
//  DE is the directory entry (DE) and is one or more zero terminated strings, one per component
//  in the DE. len is the total length of the DE.

    bool DMProcessDE(DM_CONNECTEDCOMPONENT *connectedComponent, char *DE, int len); // set the pointer to this one and extract services

//  FindService uses the service path and type to locate a directory entry if there is one.
//  If found, returns true and sets the service and component pointers appropriately.
//  If not, returns false and sets the pointers to NULL.

    bool DMFindService(SNC_UID *sourceUID, SNC_SERVICE_LOOKUP *serviceLookup);

//  DMAllocateConnectedComponent finds a spare slot in the connected component array. pData is
//  what is used for related FUL entries and is normally a pointer to the associated SS_COMPONENT.

    DM_CONNECTEDCOMPONENT *DMAllocateConnectedComponent(void *data);    // gets a spare connected component slot

//  DMDeleteConnectedComponent deletes any component entries in the directory that were received
//  from a link that is down.

    void DMDeleteConnectedComponent(DM_CONNECTEDCOMPONENT *connectedComponent);

//  DMBuildDirectoryMessage builds a complete directory from all entries if trunk is false
//  or just directly connected entries if trunk is true. The offset allows
//  the returned buffer to be sent with a header.
//  The actual directory consists of a series of zero terminated strings, each one
//   is the directory entry from a different connected component.

    void DMBuildDirectoryMessage(int offset, char **message, int *messageLength, bool trunk);

//	DMDisplay - displays directory
//	m_pLB must be set to the display dialog for Windows.

    void DMDisplay();

    char *DMGetLastErr();                                   // returns a string diagnostic

//  Tag read related routines

    bool getStartTag(const char *startTag);                 // get next tag and check that it is the one expected
    bool skipToStartTag(const char *startTag);              // get next specified start tag
    bool getEndTag(const char *endTag);                     // same for the end tag
    bool skipToEndTag(const char *endTag);                  // skip to the desired end tag
    bool getNonTag(char *str);                              // gets the  non-tag value
    bool getTag(char *tag);                                 // returns the next tag (any tag)

//  Value reading routines

    bool getSimpleBool(const char *tag, bool *boolValue);   // gets a bool value
    bool getSimpleInt(const char *tag, int *intValue);      // gets an integer value
    bool getSimpleValue(const char *tag, char *value);      // gets a string value (pValue should be of size SNC_MAX_NONTAG)
    bool getSimpleValue(const char *tag, char *value, int maxLen);  // gets a string value (value should be of size nMaxLen)

//  m_directory accesses should always use the lock

    DM_CONNECTEDCOMPONENT m_directory[SNC_MAX_CONNECTEDCOMPONENTS]; // the directory array
    QMutex m_lock;

signals:
    void DMNewDirectory(int index);
    void DMRemoveDirectory(DM_COMPONENT *component);

protected:
    DM_COMPONENT *processComponent(SNC_UID *sourceUID, char *DE, int len);// populate DM_SERVICE and DM_COMPONENT from current DE
    void invalidateServices(DM_COMPONENT *component);       // makes all service entries invalid
    void freeConnectedComponent(DM_CONNECTEDCOMPONENT *component);// frees up a connected component slot
    void deleteComponent(DM_CONNECTEDCOMPONENT *connectedComponent, DM_COMPONENT *component); // frees up a component entry
    void buildLocalDE(DM_COMPONENT *component);

    DM_COMPONENT *findComponent(DM_CONNECTEDCOMPONENT *connectedComponent, SNC_UID *UID, char *name, char *type);// finds a component in the directory given its UID

    int m_sequenceID;                                       // used to uniquely identify DEs in case they are updated in place
    char *m_tagPtr;                                         // the current pointer into the DE
    char m_lastError[SNC_MAX_TAG+SNC_MAX_NONTAG];           // a diagnostic string if an error occurs
};

#endif // DIRECTORYMANAGER_H
