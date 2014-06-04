#ifndef __clever_server_3__event__
#define __clever_server_3__event__

#include <SFML/System.hpp>
#include "inputState.h"
#include "entity.h"

enum class Evt : sf::Uint16 { nullevent, spawn, move, stateRefresh, hit };

struct cEvent {
    cEvent(): mType { Evt::nullevent } { };
    cEvent(Evt x) : mType { x } { };
    
    sf::Uint32      mLastWS;    // last relevant world state for this event
    sf::Int32       mTime;
    Evt             mType;
    sf::Uint32      mSequence;
    cInputState     mInput;
    sf::Uint16      mOwner;
    sf::Uint16      mEntityID;
    bool            mLeft, mTop;
    cEntityStatus   mTargetStatus;
    sf::Int32       mDeltaSinceLAC { 0 };
};
#endif /* defined(__clever_server_3__event__) */
