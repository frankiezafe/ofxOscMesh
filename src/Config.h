/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Config.h
 * Author: frankiezafe
 *
 * Created on June 12, 2016, 11:18 PM
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>

class Config {

public:
    static Config * get();
    
    std::string name;
    std::string names_filter;
    bool verbose;
    bool enable_osc;
    int broadcast_port;
    int osc_port;
    int heartbeat_millis;
    int max_delay_millis;
    int cam_width;
    int cam_height;
    int threshold;
    
private:
    Config();
    virtual ~Config();

};

#endif /* CONFIG_H */

