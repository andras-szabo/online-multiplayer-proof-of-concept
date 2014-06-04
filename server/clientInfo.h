#ifndef __clever_server_3__clientInfo__
#define __clever_server_3__clientInfo__

#include <SFML/Network.hpp>
#include <bitset>
#include "entity.h"

struct cClientInfo {
public:
    void                reset(sf::IpAddress, sf::Uint16, const std::string&);
    
public:
    std::string         mName { };
    sf::Uint16          mUid { 0 };
    sf::Uint16          mEntityID { 0 };

    sf::IpAddress       mAddr;
    sf::Uint16          mPort;
    sf::Int32           mRTT { 0 };
    sf::Uint32          mSequence { 0 };
    sf::Uint32          mLastAck { 0 };     // Last of our messages that we know to have been seen
    std::bitset<32>     mAcks { 0 };        // Ack bitfield
    sf::Uint8           mHandShaken { 0 };
    bool                mConnected { false };
    bool                mSynced { false };
    bool                mReady { false };
    
    sf::Int32           mLastSeenCommandTime { 0 };
    sf::Int32           mDeltaToFirstTick { 0 };

    cEntityStatus       mLastAgreedStatus;
    
    bool                mFirstDeltaSet { false };
};


#endif /* defined(__clever_server_3__clientInfo__) */
