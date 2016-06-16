/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Config.cpp
 * Author: frankiezafe
 * 
 * Created on June 12, 2016, 11:18 PM
 */

#include "Config.h"

static Config * _config = nullptr;

Config * Config::get() {
    if ( _config == nullptr ) {
        _config = new Config();
    }
    return _config;
}

Config::Config( ) {
    
    name = "undefined";
    verbose = false;
    enable_osc = true;
    broadcast_port = 20000;
    osc_port = 23000;
    heartbeat_millis = 5000;
    max_delay_millis = 20000;
    cam_width = 160;
    cam_height = 120;
    threshold = 85;
    
}

Config::~Config( ) {
}

