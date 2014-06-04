#ifndef __clever_client_2__event__
#define __clever_client_2__event__

#include <SFML/System.hpp>
#include "inputstate.h"
#include "entity.h"

enum class Evt : sf::Uint16 { nullevent, spawn, move, stateRefresh, hit };

struct cEvent {
    cEvent(): mType { Evt::nullevent } { };
    cEvent(Evt x) : mType { x } { };
    
    sf::Int32       mTime { 0 };
    Evt             mType;
    sf::Uint32      mSequence { 0 };
    cInputState     mInput;
    sf::Uint16      mOwner { 0 };       // for hit: the one that hits
    sf::Uint16      mEntityID { 0 };    // for hit: the one that's being hit

    bool            mLeft { false };    // hit from left
    bool            mTop { false };     // hit from top
    
    cEntityStatus   mTargetStatus;      // for hit events: this is where the target was
                                        // at the instance of the hit
    sf::Int32       mDeltaSinceLAC { 0 };   // time passed after last agreed status
    
};

#endif /* defined(__clever_client_2__event__) */
