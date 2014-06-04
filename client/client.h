#ifndef __clever_client_2__client__
#define __clever_client_2__client__

#include <bitset>
#include <map>
#include <queue>
#include "message.h"
#include "inputstate.h"
#include "netsim.h"
#include "worldState.h"
#include "entity.h"
#include "history.h"
#include "sceneContainer.h"
#include <SFML/Window.hpp>

struct cArchive {
    sf::Int32       mTime { 0 };
};

class cClient {
public:
    cClient(const std::string& name, sf::Uint16 protocolID, cSceneContainer& scene, sf::Uint16 port = 0);
    
    void        setup(sf::IpAddress addr,
                      sf::Uint16 ploss,
                      sf::Uint16 avgping,
                      sf::Uint16 jitter);
    
    bool        connect(sf::IpAddress, sf::Uint16);
    void        run();

private:
    void            packHeader(cMessage&, MSG, sf::Int32, sf::Int8);
    bool            receiveStuff();
    void            sendStuff();
    void            rogerThat();
    void            readyOrNot();
    
    void            incomingHandshake();
    void            incomingSync();
    void            incomingWS();
    void            incomingLobbyInfo();
    
    void            replay(const cEntityStatus&, sf::Int32, sf::Int32);
    
public:
    sf::Uint16      mProtocolID;
    sf::Uint32      mLocalSequence { 1 };
    sf::Uint16      mUid { 0 };
    std::string     mName { };
    
    double          mAvgPPS { 1 };
    sf::Uint16      mMaxPPS { 0 };
    cInputState     mCurrentInputState;

private:
    sf::RenderWindow    mWindow;
    sf::View            mView;
    cSceneContainer&    mScene;
    
    cNetSimSocket       mSocket;
    sf::Uint16          mPort;

    sf::IpAddress       mServerAddr;
    sf::Uint16          mServerPort;
    bool                mConnected { false };
    bool                mReady { false };
    bool                mSynced { false };
    
    sf::Clock           mClock;
    sf::Time            mCurrentTime { sf::Time::Zero };
    
    cEntity             mMyEntity { sf::Color::Green };
    cEntity             mSimEntity { sf::Color::Blue };
    
    sf::Uint32          mRemoteSequence { 0 };  // last seen msg from server
    sf::Uint32          mLastAck;               // last seen msg BY server
    std::bitset<32>     mAckField;
    
    bool                mMissingAck { false };
    
    cMessage            mOutMessage;
    
    cWorldState         mSRVCurrentWorldState;
    cWorldState         mSRVPreviousWorldState;
    cEntityStatus       mLastAgreedStatus;
    sf::Int32           mFirstUnackedCommandTime { 0 };
    sf::Int32           mLastAgreedCommandTime { 0 };
    sf::Int32           mFirstDelta { 0 };
    
    cEntityStatus       mNewAgreedStatus;
    sf::Int32           mIncomingFirstDelta { 0 };
    sf::Int32           mIncomingLastDelta { 0 };
    
    cHistory            mHistory;
    cISC                mIncomingISC;
    
    sf::Int32           mLastCommandCast { 0 };
    
    std::queue<cMessage>                mOutGoingMessages;
    std::queue<cMessage>                mUrgentMessages;
    std::map<sf::Int32, cMessage>       mWaitingForAck;
    std::map<sf::Uint32, cMessage>      mSent;
    std::map<sf::Int32, cInputState>    mInputBuffer;
    std::map<sf::Uint32, cArchive>      mSentArchives;      // Which message was sent at what
                                                            // time - ORIGINALLY. We need to know
                                                            // this to know when a command was
                                                            // originally cast.
    
    std::map<sf::Int32, cWorldState>    mWSonServer;        // This is where we archive incoming ws
                                                            // messages from teh server
    
    sf::Clock           mOutboxClock;
    sf::Time            mOutboxTimer { sf::Time::Zero };
    sf::Int8            mSentThisSecond { 0 };
    
    sf::Packet          inPacket;
    sf::IpAddress       inAddr;
    sf::Uint16          inPort;
    sf::Uint16          inPID;
    sf::Uint16          inUID;
    sf::Uint32          inSEQ;
    sf::Uint32          inACK;
    std::bitset<32>     inField;
    MSG                 inMSG;
        
    sf::Time            mArchiveCleanup { sf::Time::Zero };
    sf::Time            mHistoryCleanup { sf::Time::Zero };
    sf::Time            mWSACleanup { sf::Time::Zero };

    const sf::Time      xConnectTimeOut { sf::seconds(5.0) };
    const sf::Time      xPause { sf::milliseconds(5) };
    const sf::Time      xOneSec { sf::seconds(1) };
    const sf::Time      xResendTime { sf::seconds(0.4) };
    const sf::Time      xTickTime { sf::milliseconds(1000 / 30) };
    const sf::Time      xServerTickTime { sf::milliseconds(1000 / 20) };
    const sf::Int8      xMaxMessageCount { 20 };
    const sf::Time      xArchiveCleanupTime { sf::seconds(4.0) };
    const sf::Time      xHistoryTimeOut { sf::seconds(2.0) };
    const sf::Time      xHistoryLength { sf::seconds(2.0) };
    const sf::Time      xWSATimeOut { sf::seconds(2.0) };
    const sf::Time      xWSALength { sf::seconds(2.0) };
    const sf::Int8      xArchiveLength { 100 };   // messages
    const sf::Int32     xFrameTimeInMS { 1000 / 60 };   // desired fps
};

#endif /* defined(__clever_client_2__client__) */
