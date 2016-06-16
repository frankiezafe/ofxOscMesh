/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   OSCAgent.cpp
 * Author: frankiezafe
 * 
 * Created on June 12, 2016, 9:40 PM
 */

#include "OSCAgent.h"

void OSCAgent::getIPv4( map< string, string > * addresses ) {
    
    // http://stackoverflow.com/questions/212528/get-the-ip-address-of-the-machine
    
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            stringstream ss1, ss2;
            ss1 << ifa->ifa_name;
            ss2 << addressBuffer;
            if ( addresses != nullptr ) {
                (*addresses)[ ss1.str() ] = string( ss2.str() );
            }
            if ( Config::get()->verbose ) printf("ipv4 > %s : %s\n", ifa->ifa_name, addressBuffer); 
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            if ( Config::get()->verbose ) printf("ipv6 > %s : %s\n", ifa->ifa_name, addressBuffer); 
        }
    }

    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);

}

DataKind OSCAgent::makeKind( bool xy, bool xyz, bool zrot, bool matrix ) {
    DataKind k;
    for ( int i = 0; i < 8; ++i ) {
        switch( i ) {
            case 0:
                if ( xy ) { k.set( i, true ); }
                break;
            case 1:
                if ( xyz ) { k.set( i, true ); }
                break;
            case 2:
                if ( zrot ) { k.set( i, true ); }
                break;
            case 3:
                if ( matrix ) { k.set( i, true ); }
                break;
        }
    }
    return k;
}


OSCAgent::OSCAgent( DataKind k ) {
    
    map<string,string> addrs;
    getIPv4( &addrs );
    
    if ( addrs.find( "eth0" ) != addrs.end() ) {
        local_ip = addrs[ "eth0" ];
        if ( Config::get( )->verbose ) cout << "OSCAgent eth0: " << local_ip << endl;
    } else if ( addrs.find( "wlan1" ) != addrs.end() ) {
        local_ip = addrs[ "wlan1" ];
        if ( Config::get( )->verbose ) cout << "OSCAgent wlan1: " << local_ip << endl;
    } else if ( addrs.find( "wlp3s0" ) != addrs.end() ) {
        local_ip = addrs[ "wlp3s0" ];
        if ( Config::get( )->verbose ) cout << "OSCAgent wlp3s0: " << local_ip << endl;
    }
    
    // building the broadcast address
    std::size_t found = local_ip.find_last_of(".");
    broadcast_ip = local_ip.substr( 0, found + 1 ) + "255";
    
    if ( Config::get()->verbose ) {
        cout << 
            "OSCAgent config " << local_ip << ":" << Config::get()->osc_port << endl <<
            "broadcast: " << broadcast_ip << ":" << Config::get()->broadcast_port <<
            endl;
    }
    
    broadcast_sender.setup( broadcast_ip, Config::get()->broadcast_port );
    broadcast_sender.enableBroadcast();
    broadcast_receiver.setup( Config::get()->broadcast_port );
    receiver.setup( Config::get()->osc_port );
    
    lastmillis = 0;
    wait_millis = 0;
    
    kind = k;
    
}

OSCAgent::~OSCAgent( ) {
    broadcast_sender.disableBroadcast();
    for ( map< string, DataSender * >::iterator itr = senders.begin(); itr != senders.end(); ++itr ) {
        delete itr->second;
    }
}

void OSCAgent::update( uint64_t now ) {
    
    delta_millis = now - lastmillis;
    wait_millis -= delta_millis;
    lastmillis = now;
    
    if ( wait_millis <= 0 ) {
        heartbeat();
        wait_millis = Config::get()->heartbeat_millis;
    }
    
    // checking heartbeats messages
    while ( broadcast_receiver.hasWaitingMessages() ) {
        ofxOscMessage msg;
        broadcast_receiver.getNextMessage( &msg );
        heartbeat_received( now, msg );
    }
    
    // checking data messages
    while ( receiver.hasWaitingMessages() ) {
        
        ofxOscMessage msg;
        receiver.getNextMessage( &msg );
        
        // if it's a request, not sent to listeners
        if ( msg.getAddress().compare( OSC_ADDRESS_REQUEST ) == 0 ) {
            request_received( now, msg );
            break;
        }
        
        for ( vector< OSCagentListener * >::iterator itl = listeners.begin(); itl != listeners.end(); ++itl  ) {
            (*itl)->process( msg );
        }
        
        if ( Config::get()->verbose ) {
            cout << "message: " << msg.getAddress() << " from: " << msg.getRemoteIp() << ":" << msg.getRemotePort() << " containing " << msg.getNumArgs() << " value(s)" << endl;
            for ( int i = 0; i < msg.getNumArgs(); ++i ) {
                ofxOscArgType at = msg.getArgType( i );
                cout << "\t" << char( at ) << ": ";
                switch ( at ) {
                    case OFXOSC_TYPE_INT32:
                    case OFXOSC_TYPE_INT64:
                        cout << msg.getArgAsInt( i ) << ", ";
                        break;
                    case OFXOSC_TYPE_FLOAT:
                        cout << msg.getArgAsFloat( i ) << ", ";
                        break;
                    case OFXOSC_TYPE_STRING:
                        cout << msg.getArgAsString( i ) << ", ";
                        break;
                    default:
                        cout << "other arg";
                        break;
                }
                cout << endl;
            }
        }
        
    }
    
    vector< string > dead_keys;
    for ( map< string, uint64_t >::iterator itr = senders_lasttime.begin(); itr != senders_lasttime.end(); ++itr ) {
        if ( now - itr->second > Config::get()->max_delay_millis ) {
            if ( Config::get()->verbose ) {
                cout << itr->first << " is dead!!!" << endl;
            }
            dead_keys.push_back( itr->first );
        }
    }
    
    for ( vector< string >::iterator itd = dead_keys.begin(); itd != dead_keys.end(); ++itd ) {
        delete senders[ (*itd) ];
        senders.erase( (*itd) );
        senders_lasttime.erase( (*itd) );
        deletedAgentEvent( (*itd) );
    }
    
}

void OSCAgent::heartbeat() {
    
    ofxOscMessage msg;
    msg.setAddress( OSC_ADDRESS_HEARTBEAT );
    msg.addStringArg( local_ip );
    msg.addIntArg( Config::get()->osc_port );
    msg.addIntArg( int( kind.i ) );
    msg.addStringArg( Config::get()->name );
    broadcast_sender.sendMessage( msg, false );
       
    if ( Config::get()->verbose ) {
        cout << "heartbeat sent, " << int( kind.i ) << endl;
    }
    
}

void OSCAgent::heartbeat_received( uint64_t now, ofxOscMessage & msg ) {
    
    if ( msg.getAddress().compare( OSC_ADDRESS_HEARTBEAT ) != 0 ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, address not valid " << msg.getAddress() << ", should be " << OSC_ADDRESS_HEARTBEAT << endl;
        }
        return;
    }
    
    if ( msg.getNumArgs() < 2 || msg.getNumArgs() > 4 ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, received with the wrong number of args" << endl;
            cout << "\tshould be 2 or 3: string IP, int PORT, int KIND [0,9] (optional)" << endl;
        }
        return;
    }
    if ( msg.getArgType( 0 ) != OFXOSC_TYPE_STRING ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, args[0] (IP) must be a string" << endl;
        }
        return;
    }
    if ( msg.getArgType( 1 ) != OFXOSC_TYPE_INT32 && msg.getArgType( 1 ) != OFXOSC_TYPE_INT64 ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, args[1] (port) must be an integer" << endl;
        }
        return;
    }
    if ( msg.getNumArgs() > 2 && msg.getArgType( 2 ) != OFXOSC_TYPE_INT32 && msg.getArgType( 2 ) != OFXOSC_TYPE_INT64 ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, args[2] (output type) must be an integer" << endl;
        }
        return;
    }
    if ( msg.getNumArgs() > 3 && msg.getArgType( 3 ) != OFXOSC_TYPE_STRING ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::heartbeat_received, args[3] (name) must be a string" << endl;
        }
        return;
    }
    
    string bip = msg.getArgAsString( 0 );
    int bport = msg.getArgAsInt( 1 );
    int output = 0;
    string name = "undefined";
    if ( msg.getNumArgs() > 2 ) {
        output = msg.getArgAsInt( 2 );
    }
    if ( msg.getNumArgs() > 3 ) {
        name = msg.getArgAsString( 3 );
    }
    
    if ( bip.compare( local_ip ) == 0 && bport == Config::get()->osc_port ) {
        if ( Config::get()->verbose ) cout << "it's me..., " << name << " &" << output << endl;
    } else {
        if ( Config::get()->verbose ) cout << "other agent: " << bip << ":" << bport << endl;
        stringstream ss;
        ss << bip << ":" << bport;
        string unik = ss.str();
        DataKind kind( output );
        if ( senders.find( unik ) == senders.end() ) {
            createSender( unik, bip, bport, kind, name, now );
        } else {
            if ( senders[ unik ]->kind.i != kind.i ) {
                if ( Config::get()->verbose ) {
                    cout << "OSCAgent::heartbeat_received, agent " << unik << " changed kind from " << senders[ unik ]->kind.i << "to " << kind.i << " !" << endl;
                }
                senders[ unik ]->kind = kind;
            }
            if ( senders[ unik ]->name.compare( name ) != 0 ) {
                if ( Config::get()->verbose ) {
                    cout << "OSCAgent::heartbeat_received, agent " << unik << " changed name from " << senders[ unik ]->name << "to " << name << " !" << endl;
                }
                senders[ unik ]->name = name;
            }
            senders_lasttime[ unik ] = now;
        }
    }
    
}

void OSCAgent::request_received( uint64_t now, ofxOscMessage & msg ) {

    if ( msg.getAddress().compare( OSC_ADDRESS_REQUEST ) != 0 ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::request_received, address not valid " << msg.getAddress() << ", should be " << OSC_ADDRESS_REQUEST << endl;
        }
        return;
    }
    if ( 
        msg.getNumArgs() != 4 ||
        msg.getArgType( 0 ) != OFXOSC_TYPE_STRING ||
        ( msg.getArgType( 1 ) != OFXOSC_TYPE_INT32 && msg.getArgType( 1 ) != OFXOSC_TYPE_INT64 ) ||
        ( msg.getArgType( 2 ) != OFXOSC_TYPE_INT32 && msg.getArgType( 2 ) != OFXOSC_TYPE_INT64 ) ||
        ( msg.getArgType( 3 ) != OFXOSC_TYPE_INT32 && msg.getArgType( 3 ) != OFXOSC_TYPE_INT64 )
        ) {
        if ( Config::get()->verbose ) {
            cout << "OSCAgent::request_received, malformed message, args should be 0: IP(string), 1: port(int), 2: output kind(int), 3: request kind(int) " << endl;
        }
        return;
    }
    
    string bip = msg.getArgAsString( 0 );
    int bport = msg.getArgAsInt( 1 );
    int output = msg.getArgAsInt( 2 );
    int request = msg.getArgAsInt( 3 );
    
    stringstream ss;
    ss << bip << ":" << bport;
    string unik = ss.str();
    
    if ( bip.compare( local_ip ) == 0 && bport == Config::get()->osc_port ) {
        if ( Config::get()->verbose ) cout << "OSCAgent::request_received, it's me..." << endl;
    } else { 
        if ( senders.find( unik ) == senders.end() ) {
            DataKind dkind( output );
            createSender( unik, bip, bport, dkind, "undefined", now );
        }
        if ( request != senders[ unik ]->request.i ) {
            if ( Config::get()->verbose ) {
                cout << "OSCAgent::request_received, registration of request modification for " << unik << ", now " << int( senders[ unik ]->request.i ) << endl;
            }
            senders[ unik ]->request = request;
        }
        senders_lasttime[ unik ] = now;
    }
    
}

DataSender * OSCAgent::createSender( string unique,string bip, int bport, DataKind kind, string name, uint64_t now ) {
    if ( Config::get()->verbose ) { 
        cout << "new sender for " 
                << unique << " with: "
                << bip << ", "
                << bport << ", "
                << int( kind.i ) << ", "
                << name
                << endl;
    }
    DataSender * dsend = new DataSender();
    dsend->ip = bip;
    dsend->port = bport;
    dsend->out.setup( bip, bport );
    dsend->kind = kind;
    dsend->name = name;
    senders[ unique ] = dsend;
    senders_lasttime[ unique ] = now;
    newAgentEvent( unique, (*dsend) );
    return dsend;
}


void OSCAgent::send( DataSet & set ) {
    for ( map< string, DataSender * >::iterator its = senders.begin(); its != senders.end(); ++its ) {
        if ( its->second->request.i == 0 ) {
            continue;
        }
        ofxOscMessage msg;
        msg.setAddress( OSC_ADDRESS_DATA );
        msg.addStringArg( Config::get()->name );
        msg.addIntArg( set.tagid );
        msg.addIntArg( (int) set.event );
        msg.addIntArg( its->second->request.i );
        if ( its->second->request[ 0 ] ) {
            msg.addFloatArg( set.xyz.x );
            msg.addFloatArg( set.xyz.y );
        }
        if ( its->second->request[ 1 ] ) {
            msg.addFloatArg( set.xyz.x );
            msg.addFloatArg( set.xyz.y );
            msg.addFloatArg( set.xyz.z );
        }
        if ( its->second->request[ 2 ] ) {
            msg.addFloatArg( set.zrotation );
        }
        if ( its->second->request[ 3 ] ) {
            for ( int m = 0; m < 16; ++m ) {
                msg.addFloatArg( set.mat.getPtr()[ m ] );
            }
        }
        its->second->out.sendMessage( msg, false );
    }
}

void OSCAgent::request( string unique, int request_kind ) {
    
    if ( senders.find( unique ) == senders.end() ) {
        if ( Config::get()->verbose ) cout << "OSCAgent::request, no sender for " << unique << endl;
        return;
    }
    ofxOscMessage msg;
    msg.setAddress( OSC_ADDRESS_REQUEST );
    msg.addStringArg( local_ip );
    msg.addIntArg( Config::get()->osc_port );
    msg.addIntArg( int( kind.i ) );
    msg.addIntArg( request_kind );
    senders[ unique ]->out.sendMessage( msg, false );
    
}

void OSCAgent::addListener( OSCagentListener * l ) {
    if ( !containsListener( l ) ) {
        listeners.push_back( l );
    }
}

void OSCAgent::removeListener( OSCagentListener * l ) {
    for ( vector< OSCagentListener * >::iterator itl = listeners.begin(); itl != listeners.end(); ++itl  ) {
        if ( (*itl) == l ) {
            listeners.erase( itl );
            return;
        }
    }
}

void OSCAgent::removeListeners() {
    listeners.clear();
}

bool OSCAgent::containsListener( OSCagentListener * l ) {
    for ( vector< OSCagentListener * >::iterator itl = listeners.begin(); itl != listeners.end(); ++itl  ) {
        if ( (*itl) == l ) {
            return true;
        }
    }
    return false;
}

void OSCAgent::newAgentEvent( string unique, DataSender & sender ) {
    for ( vector< OSCagentListener * >::iterator itl = listeners.begin(); itl != listeners.end(); ++itl  ) {
        (*itl)->newAgent( unique, sender );
    }
}

void OSCAgent::deletedAgentEvent( string unique ) {
    for ( vector< OSCagentListener * >::iterator itl = listeners.begin(); itl != listeners.end(); ++itl  ) {
        (*itl)->deletedAgent( unique );
    }
}

string OSCAgent::toString() {

    stringstream ss;
    ss << "OSCAgent configuration: " << endl <<
        "\t" << "local ip: " << local_ip << endl <<
        "\t" << "heartbeat port: " << Config::get()->broadcast_port << endl <<
        "\t" << "data port: " << Config::get()->osc_port << endl <<
        "\t" << "heartbeat millis: " << Config::get()->heartbeat_millis << endl <<
        "\t" << "max delay millis: " << Config::get()->max_delay_millis << endl <<
        senders.size() << " registered agent(s) " << endl;
    for ( map< string, DataSender * >::iterator its = senders.begin(); its != senders.end(); ++its ) {
        ss << "\t" << its->first << endl <<
            "\t\tname: " << its->second->name << endl <<
            "\t\tip: " << its->second->ip << endl <<
            "\t\tdata port: " << its->second->port << endl <<
            "\t\tprovides: " << int( its->second->kind.i ) << endl <<
            "\t\trequest: " << int ( its->second->request.i ) << endl;
    }
    return ss.str();

    
}