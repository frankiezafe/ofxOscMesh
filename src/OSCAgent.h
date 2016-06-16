/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   OSCAgent.h
 * Author: frankiezafe
 *
 * Created on June 12, 2016, 9:40 PM
 */

#ifndef OSCAGENT_H
#define OSCAGENT_H

#include <stdio.h>      
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <cstddef> 
#include <arpa/inet.h>
#include <strstream>
#include <map>

#include "ofMain.h"
#include "Config.h"
#include "DataKind.h"
#include "OSCCommon.h"
#include "ofxOscReceiver.h"

class OSCAgent {
    
public:
    
    static void getIPv4( std::map< std::string, std::string > * addresses = nullptr );
    static DataKind makeKind( bool xy, bool xyz, bool zrot, bool matrix );
    
    OSCAgent( DataKind k );
    virtual ~OSCAgent();
    
    void update( uint64_t now);
    void send( DataSet & set );
    void request( string unique, int request_kind );
    
    // getters
    DataKind & getOutputKind() { return kind; }
    
    // setters
    inline void setKind( DataKind k ) { kind = k; }
    
    // listeners
    void addListener( OSCagentListener * l );
    void removeListener( OSCagentListener * l );
    void removeListeners();
    
    // utils
    string toString();
    
protected:

    uint64_t lastmillis;
    int delta_millis;
    int wait_millis;
    string local_ip;
    string broadcast_ip;
    DataKind kind;
    
    ofxOscSender broadcast_sender;
    ofxOscReceiver broadcast_receiver;
    
    map< string, DataSender * > senders;
    map< string, uint64_t > senders_lasttime;
    ofxOscReceiver receiver;
    
    void heartbeat();
    void heartbeat_received( uint64_t now, ofxOscMessage & msg );
    void request_received( uint64_t now, ofxOscMessage & msg );
    DataSender * createSender( string unique, string bip, int bport, DataKind kind, string name, uint64_t now );
    bool containsListener( OSCagentListener * l );
    
    // listeners
    vector< OSCagentListener * > listeners;
    void newAgentEvent( string unique, DataSender & sender );
    void deletedAgentEvent( string unique );
    
};

#endif /* OSCAGENT_H */

