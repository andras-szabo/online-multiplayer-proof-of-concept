#include "clientInfo.h"

void cClientInfo::reset(sf::IpAddress addr, sf::Uint16 port, const std::string& name)
{
    mName = name;
    mPort = port;
    mAddr = addr;
    
    mRTT = 0;
    mSequence = 0;
    mLastAck = 0;
    mAcks = 0;
    mHandShaken = 0;
    mConnected = false;
    mSynced = false;
    mReady = false;
    
    mLastSeenCommandTime = 0;
    mDeltaToFirstTick = 0;
}
