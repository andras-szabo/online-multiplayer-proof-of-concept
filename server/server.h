#ifndef __clever_server_3__server__
#define __clever_server_3__server__

#include "message.h"
#include "netsim.h"
#include "clientInfo.h"
#include "history.h"
#include "worldState.h"
#include "entity.h"
#include "clientHolder.h"
#include "sceneContainer.h"
#include <SFML/Window.hpp>

class cServer {
public:
    cServer(sf::Uint16 protocolID, sf::Uint16 expectedclients,
            sf::Uint16 port = 0, cSceneContainer* scene = nullptr);
    
    void        run();
    
public:
    sf::Uint16      mProtocolID;
    sf::Uint32      mLocalSequence { 1 };
    sf::Time        mPause { sf::milliseconds(5) };
    
    cSceneContainer*    pScene { nullptr };
    
private:
    void            sendStuff();
    bool            receiveStuff();
    bool            rogerThat();
    void            syncTime();
    void            packHeader(cMessage&, sf::Uint16, MSG, sf::Int32, sf::Uint16);
    
    void            tick(sf::Int32);
    inline void     sync(sf::Int32&, sf::Uint16);
    void            broadcastWorldState(sf::Int32);
    
    void            moveEntity(const cEvent&);
    
    void            refreshSent();
    
    void            incomingEvents();
    void            incomingFirstContact();
    void            incomingHandshake();
    void            incomingSync();
    void            incomingReady();
    void            incomingUnready();
    
    void            broadcastLobbyInfo();
    
    void            setupStage();
    
private:    
    cClientHolder                       mClient;
    sf::RenderWindow                    mWindow;
    sf::View                            mView;
    cNetSimSocket                       mSocket;
    sf::Uint16                          mPort;
    
    sf::Uint16                          mExpectedClients { 0 };
    bool                                mEveryoneSynced { false };
    bool                                mSyncAttempted { false };
    
    sf::Clock                           mClock;
    sf::Time                            mCurrentTime;
    sf::Time                            mLastTickTime;
        
    std::map<sf::Int32, cMessage>       mWaitingForAck;
    std::map<sf::Uint32, cMessage>      mSent;
    std::queue<cMessage>                mUrgentMessages;
    std::queue<cMessage>                mOutGoingMessages;
    cHistory                            mHistory;
    
    cISC                                mISC;       // fixed-size array for keeping track
                                                    // of input changes in the latest tick
    // Given that latest have to be saved and re-saved every tick, I think it's better to
    // use a fixed-size array with a counter that always shows how many items it contains at
    // any given moment, than to use a vector or some other structure which would need to
    // be allocated and freed each and every tick, regardless of what did (not) happen.
    
    cWorldState*                        pCurrentWorldState { nullptr };
    cWorldState*                        pPreviousWorldState { nullptr };
    
    std::vector<cEvent>                 mPreviousHitQueue { };
    
    sf::Packet                          inPacket;
    sf::IpAddress                       inAddr;
    sf::Uint16                          inPort;
    sf::Uint16                          inPID;
    sf::Uint16                          inUID;
    sf::Uint32                          inSEQ;
    sf::Uint32                          inACK;
    sf::Int32                           inRTT;
    MSG                                 inMSG;
    
    cMessage                            outMessage;
    
    sf::Time                            mLastSentCleanup { sf::Time::Zero };
    sf::Time                            mLastHistoryCleanup { sf::Time::Zero };
    sf::Time                            mLastWSACleanup { sf::Time::Zero };
    sf::Time                            mLastEvtSequenceCleanup { sf::Time::Zero };
    
    sf::Uint8                           mSentThisSecond;
    sf::Time                            mOutboxTimer { sf::Time::Zero };
    sf::Clock                           mOutboxClock;
    
    const sf::Uint8                     xMaxMessageCount { 100 };   // max # of messages in a second;
    const sf::Int8                      xHandshakeNeeded { 8 };
    const sf::Int32                     xSuspiciousRTT { 250 };
    
    const sf::Time                      xOneSec { sf::seconds(1.0) };
    const sf::Time                      xResendTime { sf::seconds(0.4) };
    const sf::Time                      xSentTimeOut { sf::seconds(4.0) };
    const sf::Time                      xTickTime { sf::milliseconds(1000 / 20) };
    const sf::Time                      xHistoryTimeOut { sf::seconds(4.0) };
    const sf::Time                      xHistoryLength { sf::seconds(6.0) };
    const sf::Time                      xWSATimeOut { sf::seconds(4.0) };
    const sf::Time                      xWSALength { sf::seconds(6.0) };
    
    const sf::Int32                     xFrameTimeInMs { 1000 / 60 };
};


#endif /* defined(__clever_server_3__server__) */
