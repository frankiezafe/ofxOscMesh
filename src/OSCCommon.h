/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   OSCCommon.h
 * Author: frankiezafe
 *
 * Created on June 14, 2016, 8:53 PM
 */

#ifndef OSCCOMMON_H
#define OSCCOMMON_H

#include "ofMain.h"
#include "Config.h"
#include "ofxOscSender.h"
#include "DataKind.h"

#define OSC_ADDRESS_HEARTBEAT   "/a"
#define OSC_ADDRESS_REQUEST     "/r"
#define OSC_ADDRESS_DATA        "/d"

enum DataEvent {
    TAG_APPEAR =        0,
    TAG_PRESENT =       1,
    TAG_DISAPPEARED =   2
};

struct DataSet {
    uint16_t tagid;
    ofVec3f xyz;
    float zrotation;
    ofMatrix4x4 mat;
    DataEvent event;
};

struct DataSender {
    ofxOscSender out;
    string ip;
    uint16_t port;
    string name;
    DataKind kind; // defines what the agent can provide
    // if request.i == 0 >> nothing is sent
    // if request[ 0 ] > sending xy coordinates of the tag
    // if request[ 1 ] > sending xyz coordinates of the tag
    // if request[ 2 ] > sending rotation Z of the tag
    // if request[ 3 ] > sending matrix of the tag
    // if request[ 4 ] > free slot
    // if request[ 5 ] > free slot
    // if request[ 6 ] > free slot
    // if request[ 7 ] > free slot
    DataKind request;
    
};

class OSCagentListener {
public:
    virtual void process( ofxOscMessage & msg ) = 0;
    virtual void newAgent( string unique, DataSender & sender ) = 0;
    virtual void deletedAgent( string unique ) = 0;
};

#endif /* OSCCOMMON_H */

