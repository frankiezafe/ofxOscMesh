/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataKind.h
 * Author: frankiezafe
 *
 * Created on June 14, 2016, 8:51 PM
 */

#ifndef DATAKIND_H
#define DATAKIND_H

class DataKind {
    
public:
    
    const int & i;
    const std::bitset< 8 > & bits;
    
    DataKind(): i(_i), bits(_bits) {
        _i = 0;
        render_bits();
    }
    
    DataKind( int value ): i(_i), bits(_bits) {
        _i = value;
        render_bits();
    }
    
    inline void set( int pos, bool v ) {
        if ( v ) { _bits[ pos ] = 1; }
        else { _bits[ pos ] = 0; }
        render_int();
    }
    
    bool operator [] ( int pos ) {
        if ( _bits[ pos ] == 1 ) {
            return true;
        }
        return false;
    }
    
    void operator = ( int value ) {
        _i = value;
        render_bits();
    }
    
    void operator = ( std::bitset< 8 > & value ) {
        for ( int n = 0; n < 8; ++n ) {
            _bits[ n ] = value[ n ];
        }
        render_int();
    }
    
    void operator = ( DataKind value ) {
        _i = value.i;
        render_bits();
    }
    
    bool operator == ( DataKind value ) {
        return ( _i == value.i );
    }
    
    bool operator != ( DataKind value ) {
        return ( _i != value.i );
    }
    
protected:
    
    int _i;
    std::bitset< 8 > _bits;
    
    void render_bits() {
        std::bitset< 8 > tmpb( _i );
        for ( int n = 0; n < 8; ++n ) {
            _bits[ n ] = tmpb[ n ];
        }
    }
    
    void render_int() {
        _i = (int) _bits.to_ulong();
    }

};


#endif /* DATAKIND_H */

