#include <iostream>
#include "client.h"
#include "helpers.h"

bool operator==(const cInputState& a, const cInputState& b)
{
    return ( a.mLeft == b.mLeft && a.mRight == b.mRight && a.mUp == b.mUp && a.mPunch == b.mPunch && a.mBlock == b.mBlock );
}

bool operator!=(const cInputState& a, const cInputState& b)
{
    return !(a==b);
}

bool operator==(const cEntityStatus& a, const cEntityStatus& b)
{
    return ( a.mCurrentInputState == b.mCurrentInputState &&
            a.mLastUpdated == b.mLastUpdated &&
            a.mPos == b.mPos );
}

bool operator!=(const cEntityStatus& a, const cEntityStatus& b)
{
    return !(a==b);
}

sf::Packet& operator>>(sf::Packet& p, sf::Vector2f& v)
{
    return p >> v.x >> v.y;
}

sf::Packet& operator>>(sf::Packet& p, std::bitset<32>& b)
{
    sf::Uint32 u;
    p >> u;
    b = static_cast<std::bitset<32>>(u);
    return p;
}

sf::Packet& operator>>(sf::Packet& p, MSG& m)
{
    sf::Uint16  in;
    p >> in;
    m = static_cast<MSG>(in);
    return p;
}

sf::Packet& operator>>(sf::Packet& p, cInputState& is)
{
    return p >> is.mLeft >> is.mRight >> is.mUp >> is.mPunch >> is.mBlock;
}

sf::Packet& operator>>(sf::Packet& p, cInputStateChange& is)
{
    return p >> is.mDelta >> is.mOwner >> is.mEntityID >> is.mInput;
}

sf::Packet& operator>>(sf::Packet& p, cEntityStatus& e)
{
    sf::Uint16 state;
    p >> e.mOwner >> e.mUid >> e.mCurrentInputState >> e.mLastUpdated >> state;
    e.mState = static_cast<EntState>(state);
    return p >> e.mPos >> e.mVel >> e.mStatePtr >> e.mInAir >> e.mIsPunching >> e.mHitAlready >> e.mHitNeedsRelease;
}

sf::Packet& operator>>(sf::Packet& p, cEvent& event)
{
    sf::Uint16 tmp;
    
    p >> tmp >> event.mTime >> event.mOwner >> event.mEntityID;
    
    event.mType = static_cast<Evt>(tmp);
    
    switch ( event.mType ) {
        case Evt::move: {
            
            p >> event.mInput;
            break;
            
        }
        case Evt::hit: {

            p >> event.mLeft >> event.mTop >> event.mTargetStatus >> event.mDeltaSinceLAC;
            break;
        }
            
        default:
            break;
    }
    return p;
}

sf::Packet& operator>>(sf::Packet& p, cWorldState& ws)
{
    p >> ws.mEntityCount;
    
    ws.mEntities.clear();
    for ( auto i = 0; i < ws.mEntityCount; ++i )
    {
        cEntityStatus   ent;
        p >> ent;
        ws.mEntities.push_back(std::move(ent));
    }

    return p >> ws.mTime;
}

////

sf::Packet& operator<<(sf::Packet& p, sf::Vector2f& v)
{
    return p << v.x << v.y;
}

sf::Packet& operator<<(sf::Packet& p, cInputState& is)
{
    return p << is.mLeft << is.mRight << is.mUp << is.mPunch << is.mBlock;
}

sf::Packet& operator<<(sf::Packet& p, cEntityStatus& s)
{
    p << s.mOwner << s.mUid << s.mCurrentInputState << s.mLastUpdated << static_cast<sf::Uint16>(s.mState);
    return p << s.mPos << s.mVel << s.mStatePtr << s.mInAir << s.mIsPunching << s.mHitAlready << s.mHitNeedsRelease;
}

sf::Packet& operator<<(sf::Packet& p, cWorldState& ws)
{
    p << ws.mEntityCount;
    
    for ( auto& i : ws.mEntities )
        p << i;
    
    return p << ws.mTime;
    
}

sf::Packet& operator<<(sf::Packet& p, std::bitset<32>& bits)
{
    return p << static_cast<sf::Uint32>(bits.to_ulong());
}

sf::Packet& operator<<(sf::Packet& p, MSG& m)
{
    return p << static_cast<sf::Uint16>(m);
}

sf::Packet& operator<<(sf::Packet& p, cInputStateChange& is)
{
    return p << is.mDelta << is.mOwner << is.mEntityID << is.mInput;
}

sf::Packet& operator<<(sf::Packet& p, cEvent& e)
{
    p << static_cast<sf::Uint16>(e.mType) << e.mTime << e.mOwner << e.mEntityID;
    
    switch ( e.mType ) {
        case Evt::move: {
            p << e.mInput;
            break;
        }

        case Evt::hit: {
            p << e.mLeft << e.mTop << e.mTargetStatus << e.mDeltaSinceLAC;
            break;
        }
            
        default:
            break;
    }

    return p;
}

//////

cClient::cClient(const std::string& name, sf::Uint16 protocolID, cSceneContainer& scene, sf::Uint16 port):
mScene { scene },
mName { name },
mProtocolID { protocolID },
mPort { port }
{
    if (mSocket.bindPort(mPort) != true )
    {
        throw std::runtime_error("Couldn't bind socket to port.");
    }
    
    std::cout << "Client ready and waiting, bound to port " << mPort << ".\n";
    
    mWindow.create(sf::VideoMode(320, 240), "Client" );
    
    mView.setSize(640,480);
    mView.setCenter(320,240);
    mView.setViewport(sf::FloatRect(0,0,1,1));
    mWindow.setView(mView);
    
    mWindow.setVerticalSyncEnabled(true);
    mWindow.clear();
}

bool cClient::connect(sf::IpAddress addr, sf::Uint16 sport)
{
    mServerAddr = addr;
    mServerPort = sport;
    auto attempts = 0;
    
    while ( !mConnected && attempts < 3 )
    {
        packHeader(mOutMessage, MSG::firstContact, mClock.getElapsedTime().asMilliseconds(), 0);
        mOutMessage.mPacket << mName;
        mOutGoingMessages.push(mOutMessage);
        
        sf::Time        trying { sf::Time::Zero };
        sf::Clock       tmpClock;
        while ( !mConnected && trying < xConnectTimeOut )
        {
            receiveStuff();
            sendStuff();
            sf::sleep(xPause);
            trying += tmpClock.restart();
        }
        ++attempts;
    }
    
    return mConnected;
}

void cClient::packHeader(cMessage & m, MSG mtype, sf::Int32 time, sf::Int8 ttl)
{
    m.mPacket.clear();
    m.mPacket << mProtocolID << mUid << mLocalSequence << mRemoteSequence << mtype;
    m.mAddr = mServerAddr;
    m.mSequence = mLocalSequence;
    m.mTimeSent = time;
    m.mTTL = ttl;
    ++mLocalSequence;
}

bool cClient::receiveStuff()
{
    bool recvd { false };
    if ( mSocket.recv(inPacket, inAddr, inPort) )
    {
        // Read header, which is always the same:
        inPacket >> inPID >> inUID >> inSEQ >> inACK >> inField >> inMSG;
        
        if ( inPID == mProtocolID && later(inSEQ, mRemoteSequence) )
        {

            mRemoteSequence = inSEQ;
            rogerThat();
            recvd = true;
            
            switch ( inMSG ) {
                case MSG::handshake: {
                    incomingHandshake();
                    break;
                }
                case MSG::lobbyInfo: {
                    incomingLobbyInfo();
                    break;
                }
                case MSG::syncClocks: {
                    incomingSync();
                    break;
                }
                case MSG::worldState: {
                    incomingWS();
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            // wrong protocol number, or older remote sequence
        }
    }

    return recvd;
}

void cClient::rogerThat()
{
    // Maintain mWaitingForAck and mSent.
    // Expects the latest ackfield to be in inField, the previous
    // one in mAckField, previous latest ack in mLastAck, new latest
    // ack in inACK.
    
    // To know which are the newly acked messages:
    // shift previous ackfield to the left an appropriate number of steps,
    // so that it aligns with the new one,
    // then XOR with new one
    // then iterate through the thing and remove 'em from Sent and WFA,
    // then let old ackfield = new ackfield, old lastAck = new lastAck.
    
    // Also maintains mMissingAck: this shows if we're missing acks for
    // messages BEFORE the last acked one. If we do, then we can't accept
    // the server's version of things as authoritative: there's a
    // possibility that they will in fact be different.
    
    if ( !later(inACK, mLastAck) && ( inACK != mLastAck ) )
    {
        return;
    }
    
    sf::Uint32 i = inACK - mLastAck;
    mAckField <<= i;
    
    mAckField = mAckField ^ inField;
    
    // Acking current:
    auto it = mSent.find(inACK);
    if ( it != mSent.end() )
    {
        mWaitingForAck.erase(it->second.mTimeSent);
        mSent.erase(it);
    }
    
    // Acking past:
    for ( i = 0; i < 32; ++i )
    {
        if ( mAckField[i] == 1 )
        {
            auto it = mSent.find(inACK - ( 1 + i ));
            if ( it != mSent.end() )
            {
                mWaitingForAck.erase(it->second.mTimeSent);
                mSent.erase(it);
            }
        }
    }
    
    mAckField = inField;
    mLastAck = inACK;
    
    // Look for missing acks
    for ( auto i = 0; i < ( mLastAck-1 < 32 ? mLastAck-1 : 32 ); ++i )
    {
        if ( mAckField[i] == 0 )
        {
            // No ack received for sequence mLastAck-1-i
            // std::cout << "Missing ack for: " << mLastAck-i << "\n";
            mMissingAck = true;
            return;
        }
    }
    
    mMissingAck = false;
}
    
void cClient::readyOrNot()
{
    if ( mReady )
    {
        packHeader(mOutMessage, MSG::ready, mClock.getElapsedTime().asMilliseconds(), 3);
    }
    else
    {
        packHeader(mOutMessage, MSG::unready, mClock.getElapsedTime().asMilliseconds(), 3);
    }
    
    mOutGoingMessages.push(mOutMessage);
}
    
void cClient::incomingLobbyInfo()
{
    std::cout << "My Uid is: " << mUid << "\n";
    std::cout << "Incoming lobby info. Connected are: \n";
    sf::Uint16 count;
    inPacket >> count;
    for ( auto i = 0; i < count; ++i )
    {
        std::string inName;
        sf::Int32   inPing;
        bool        inReady;
        
        inPacket >> inName >> inPing >> inReady;
        std::cout << inName << "; RTT: " << inPing << (inReady ? "; Ready " : "; Not ready ");
        if ( inName == mName )
        {
            std::cout << "<- that's me!";
        }
        std::cout << "\n";
    }
    
    mConnected = true;
    
    // Ack lobby info by sending out current ready / not ready status
    // Also: ready itself, if not done so before.
    
    if ( !mReady )
    {
        mReady = true;
        readyOrNot();
    }
}

void cClient::incomingHandshake()
{
    mUid = inUID;
    mScene.myOwnUid = mUid;
    packHeader(mOutMessage, MSG::handshake, mClock.getElapsedTime().asMilliseconds(), 3);
    mUrgentMessages.push(mOutMessage);  // Handshakes are urgent b/c they help establish RTT
}

void cClient::incomingSync()
{
    
    // Upon receipt of sync message: restart clock, and send ack
    mClock.restart();
    packHeader(mOutMessage, MSG::syncClocks, mClock.getElapsedTime().asMilliseconds(), 6);
    mUrgentMessages.push(mOutMessage);
    mSynced = true;
}
    
void cClient::replay(const cEntityStatus& status,
                     sf::Int32 srvFirstDelta,
                     sf::Int32 currentTime)
{

    mScene.rewindWorldTo(status.mLastUpdated);          // rewinds the world, but not the player
    mScene.setPlayerStatus(status);                     // rewinds own player entity
    mScene.updatePlayerWithoutDelta(-srvFirstDelta);    // align client and server time
    
    // Then go back in time, and reapply events and all.
    
    sf::Int32   updatePtr { mScene.mCurrentWorldState.mTime };
    cEvent      nextEvent { mHistory.getNextEvent(updatePtr - 1) };
    sf::Int32   nextFrameTime { updatePtr + xFrameTimeInMS };
    sf::Int32   lastFrameTime { 0 };
    bool        firstWSpassed { false };
    
    while ( nextFrameTime <= currentTime ||
          ( nextEvent.mType != Evt::nullevent && nextEvent.mTime <= currentTime ) )
    {
        if ( nextEvent.mType == Evt::nullevent || nextFrameTime < nextEvent.mTime )
        {
            mScene.updateAllAt(nextFrameTime, true);
            lastFrameTime = nextFrameTime;
            nextFrameTime += xFrameTimeInMS;
        }
        else
        {
            if ( nextEvent.mType == Evt::stateRefresh )
            {
                if ( firstWSpassed == false )
                {
                    firstWSpassed = true;
                }
                else
                {
                    mScene.handle(nextEvent);
                }
            }
            else
            {
                mScene.handle(nextEvent);
            }

            updatePtr = nextEvent.mTime;
            nextEvent = mHistory.getNextEvent(updatePtr);

        }
    }
    
    if ( lastFrameTime < currentTime )
    {
        mScene.updateAllAt(currentTime, true);
    }

}
    
void cClient::incomingWS()
{
    sf::Int32   currentTime  { mClock.getElapsedTime().asMilliseconds() };
    
    inPacket >> mSRVPreviousWorldState >> mSRVCurrentWorldState;
    inPacket >> mIncomingISC.mItemCount;
        
    for ( auto i = 0; i < mIncomingISC.mItemCount; ++i )
    {
        inPacket >> mIncomingISC.mItem[i];
    }
    
    inPacket >> mNewAgreedStatus >> mIncomingFirstDelta;
    
    // So first: do we have a new LAS?
    
    bool newLAS { false };
    
    // Unfortunately we have to accept new LAS even if it's made on the basis
    // of insufficient information (mMissingAck might be true).
    
    if ( mNewAgreedStatus != mScene.mLastAgreedPlayerStatus ) // && !mMissingAck )
    {
        newLAS = true;
        mScene.mLastAgreedPlayerStatus = mNewAgreedStatus;
        mLastAgreedCommandTime = mSentArchives[inACK].mTime;
        mScene.mLastAgreedCommandTime = mLastAgreedCommandTime;
    }
    
    // Then: let's add every incoming hit event to history.
    
    sf::Uint16 incomingHitCount;
    inPacket >> incomingHitCount;
    
    // Read incoming *fresh* hit events, and add them to the history of the future
    
    cEvent hitEvent { Evt::hit };
    
    bool hitInThePast { false };
    
    for ( ; incomingHitCount > 0; --incomingHitCount )
    {
        inPacket >> hitEvent;
        
        // If the incoming hit affects others, no worries, we add them to the present,
        // otherwise, to the "past"
        
        if ( hitEvent.mEntityID != mUid )
        {
            hitEvent.mTime += currentTime;
            mHistory.add(hitEvent);
        }
        else
        {
            hitEvent.mTime = mScene.mLastAgreedCommandTime + hitEvent.mDeltaSinceLAC;
            hitInThePast = true;
            mHistory.add(hitEvent);
        }
    }
    
    // Now read *archived* hit events; ones that supposedly were already filed, but
    // were sent over again, just in case the previous world state was lost.
    
    inPacket >> incomingHitCount;
    
    for ( ; incomingHitCount > 0; --incomingHitCount )
    {
        inPacket >> hitEvent;
        
        // If we find a hit event that's already added, that means we already added these
        // all, so no point in going on. Otherwise - add 'em all.
        
        if ( hitEvent.mEntityID != mUid )
        {
            hitEvent.mTime += currentTime;
            if ( !mHistory.alreadyAdded(hitEvent) )
            {
                mHistory.add(hitEvent);
            }
            else
            {
                incomingHitCount = 1;
            }
        }
        else
        {
            hitEvent.mTime = mScene.mLastAgreedCommandTime + hitEvent.mDeltaSinceLAC;
            if ( !mHistory.alreadyAdded(hitEvent) )
            {
                hitInThePast = true;
                mHistory.add(hitEvent);
            }
            else
            {
                incomingHitCount = 1;
            }
        }
    }

    // OK. Now,
    // if we have a new LAS or a hit in the past, we'll have to rewind things
    
    if ( newLAS || hitInThePast )
    {
        cEntityStatus tmp { mScene.mLastAgreedPlayerStatus };
        tmp.mLastUpdated = mLastAgreedCommandTime;
        replay(tmp, mIncomingFirstDelta, currentTime);
    }
    
    // Finally, consider forthcoming events of others.
    
    cEvent      event { Evt::stateRefresh };
    event.mTime = currentTime;
    mHistory.add(event);
    
    // When saving world states, we need to sync their times to match that on
    // the client!
    
    mSRVPreviousWorldState.updateTime(event.mTime);
    mWSonServer[event.mTime] = mSRVPreviousWorldState;
    
    // Let the scene know where we're at
    
    mScene.setWorldState(mSRVPreviousWorldState);
    
    event.mTime += xServerTickTime.asMilliseconds();
    
    mSRVCurrentWorldState.updateTime(event.mTime);
    mWSonServer[event.mTime] = mSRVCurrentWorldState;
        
    // Then add incoming input changes as events to future history;
    
    event.mType = Evt::move;
    for ( auto i = 0; i < mIncomingISC.mItemCount; ++i )
    {
        event.mInput = mIncomingISC.mItem[i].mInput;
        event.mTime = currentTime + mIncomingISC.mItem[i].mDelta;
        event.mOwner = mIncomingISC.mItem[i].mOwner;
        event.mEntityID = mIncomingISC.mItem[i].mEntityID;
        mHistory.add(event);
    }
    
}

void cClient::run()
{
    sf::Time        timetmp { sf::Time::Zero };
    
    sf::Clock       loopClock;
    cInputState     prevState;
    bool            updatedAlready;
    
    cEvent          nextEvent { mHistory.getNextEvent(mCurrentTime.asMilliseconds()) };
    sf::Time        pastFrameTick { sf::Time::Zero };
    
    bool cnt { true };
    while ( cnt )
    {
        timetmp = loopClock.restart();
        mCurrentTime = mClock.getElapsedTime();
        
        // Let's see if there's a "future" event we have to deal with.
        
        nextEvent = mHistory.getNextEvent(pastFrameTick.asMilliseconds());
        
        while ( nextEvent.mType != Evt::nullevent && mCurrentTime.asMilliseconds() >= nextEvent.mTime )
        {
            switch ( nextEvent.mType ) {
         
                case Evt::stateRefresh: {
                    mScene.setWorldState(mWSonServer[nextEvent.mTime]);
                    break;
                }
                
                case Evt::move: {
                    mScene.handle(nextEvent);
                    break;
                }
                    
                case Evt::hit: {
                    mScene.handle(nextEvent);
                    break;
                }
                default: break;
            }
            
            nextEvent = mHistory.getNextEvent(nextEvent.mTime);
        }
        
        pastFrameTick = mCurrentTime;
        
        // Input check
        
        mCurrentInputState.mLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
        mCurrentInputState.mRight = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
        mCurrentInputState.mUp = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
        mCurrentInputState.mPunch = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
        mCurrentInputState.mBlock = sf::Keyboard::isKeyPressed(sf::Keyboard::Q);
        
        cnt = !sf::Keyboard::isKeyPressed(sf::Keyboard::Escape);
        
        if ( mCurrentInputState != prevState )
        {
            bool irrelEvent { false };
            // Check for irrelevant events
            if ( !mCurrentInputState.mPunch && prevState.mPunch
                 && mCurrentInputState.mLeft == prevState.mLeft
                 && mCurrentInputState.mRight == prevState.mRight
                 && mCurrentInputState.mBlock == prevState.mBlock
                 && mCurrentInputState.mUp == prevState.mUp )   // punch released, nothing else changed
            {
                irrelEvent = true;
            }
            
            if ( !mCurrentInputState.mUp && prevState.mUp
                && mCurrentInputState.mLeft == prevState.mLeft
                && mCurrentInputState.mRight == prevState.mRight
                && mCurrentInputState.mBlock == prevState.mBlock
                && mCurrentInputState.mPunch == prevState.mPunch )   // jump released, nothing else changed
            {
                irrelEvent = true;
            }
            
            
            if ( mCurrentInputState.mLeft && mCurrentInputState.mRight )
            {
                if ( prevState.mLeft )    // the previous one was left
                {
                    mCurrentInputState.mLeft = false;
                } else
                {
                    mCurrentInputState.mRight = false;
                }
            }
            
            cEvent      mevent(Evt::move);
            mevent.mInput = mCurrentInputState;
            prevState = mCurrentInputState;
            
            mevent.mTime = mClock.getElapsedTime().asMilliseconds();
            mevent.mEntityID = mUid;
            mevent.mOwner = mUid;

            mHistory.add(mevent);

            // Set player state
            mScene.setPlayerInputStateAt(mevent.mInput, mevent.mTime);

            // Update everything at this very moment
            mScene.updateAllAt(mevent.mTime);
            updatedAlready = true;
            
            // Save world snapshot
            mScene.archiveCurrentWorldState(mevent.mTime);
            
            // And send event immediately - IF relevant:
            if ( !irrelEvent )
            {
                packHeader(mOutMessage, MSG::event, mevent.mTime, 3);
                mOutMessage.mPacket << static_cast<sf::Uint16>(1);      // Number of events: 1
                mOutMessage.mPacket << mevent;
                mOutMessage.mFirstDelta = 0;
                mUrgentMessages.push(mOutMessage);
                
                // saving event in sent archives
                
                cArchive tmp;
                tmp.mTime = mevent.mTime;
                mSentArchives[mLocalSequence - 1] = std::move(tmp);
            }
        }
        
        // Send and receive what needs to be sent and received
        
        sendStuff();
        receiveStuff();
        
        // Update folks; for now: always update mMyEntity!
        
        if ( !updatedAlready )
        {
            mScene.updateAllAt(mClock.getElapsedTime().asMilliseconds());
        }
        else
        {
            updatedAlready = false;
        }
        
        
        // Render
        mWindow.clear();
        
        mScene.render(mWindow);
    
        mWindow.display();
        
        // Cleanup
        
        mArchiveCleanup += timetmp;
        if ( mArchiveCleanup >= xArchiveCleanupTime && mLocalSequence > xArchiveLength )
        {
            mArchiveCleanup -= xArchiveCleanupTime;
            mSentArchives.erase(mSentArchives.begin(),
                                mSentArchives.upper_bound(mLocalSequence - xArchiveLength));
        }
        
        mHistoryCleanup += timetmp;
        if ( mHistoryCleanup >= xHistoryTimeOut )
        {
            mHistoryCleanup -= xHistoryTimeOut;
            mHistory.cleanup((mClock.getElapsedTime() - xHistoryLength).asMilliseconds());
        }
        
        mWSACleanup += timetmp;
        if ( mWSACleanup >= xWSATimeOut )
        {
            mWSACleanup -= xWSATimeOut;
            mWSonServer.erase(mWSonServer.begin(),
                              mWSonServer.lower_bound((mClock.getElapsedTime()-xWSALength).asMilliseconds()));

            mScene.archiveCleanup();
        }
        
        sf::sleep(sf::milliseconds(5));
    }
    
    std::cout << "Avg PPS (Packet Per Sec): " << mAvgPPS << "\n";
    std::cout << "Max PPS: " << mMaxPPS << "\n";
    
}

void cClient::setup(sf::IpAddress addr, sf::Uint16 ploss, sf::Uint16 avgping, sf::Uint16 jitter)
{
    mSocket.setAddress(addr);
    mSocket.setAvgPing(avgping);
    mSocket.setJitter(jitter);
    mSocket.setPacketLoss(ploss);
}

void cClient::sendStuff()
{
    // First: send urgent messages
    
    while ( !mUrgentMessages.empty() )
    {
        mCurrentTime = mClock.getElapsedTime();
        mOutMessage = mUrgentMessages.front();
        mUrgentMessages.pop();
        mOutMessage.mTimeSent = mCurrentTime.asMilliseconds();
        
        mSocket.send(mOutMessage.mPacket, mServerAddr, mServerPort);
        
        // Reliability: send it again!; but try to keep the number of
        // packets as low as possible:
        
        bool resend { false };
        
        if ( mSentThisSecond < xMaxMessageCount )
        {
            mSocket.send(mOutMessage.mPacket, mServerAddr, mServerPort);
            resend = true;
        }
        
        mSent[mOutMessage.mSequence] = mOutMessage;
        
        if ( mOutMessage.mTTL != 0 )
        {
            mWaitingForAck[mCurrentTime.asMilliseconds()] = mOutMessage;
        }
        
        mSentThisSecond += (1 + static_cast<int>(resend));  // either 1 or 2 new messages in this second
    }
    
    
    mOutboxTimer += mOutboxClock.restart();
    if ( mOutboxTimer >= xOneSec )
    {
        if ( mSentThisSecond > 0 )
        {
            mAvgPPS += mSentThisSecond;
            mAvgPPS /= 2;
            
            if ( mMaxPPS < mSentThisSecond )
                mMaxPPS = mSentThisSecond;
        }
        mSentThisSecond = 0;
        mOutboxTimer -= xOneSec;
    }
        
    // OK, so first: packet resend, if any unacked packets that have been waiting for
    // a long time. But in every iteration of sendstuff(), we only resend 1 packet.
        
    if ( !mWaitingForAck.empty() && mSentThisSecond < xMaxMessageCount )
    {
        mCurrentTime = mClock.getElapsedTime();
        if ( mCurrentTime.asMilliseconds() - std::begin(mWaitingForAck)->first >= xResendTime.asMilliseconds() )
        {
            // Looks like a packet (or packets) here was lost. So let's resend it.
            // First, remove message from waitinforack, and modify Sent so that we
            // keep track of sending time:
                
            cMessage m = mWaitingForAck.begin()->second;
            
            mWaitingForAck.erase(mWaitingForAck.begin());
            mSent[m.mSequence].mTimeSent = mCurrentTime.asMilliseconds();
                
            mSocket.send(m.mPacket, mServerAddr, mServerPort);
                
            // If the message has still time to live, then add it back to waitingForAck
            if ( --m.mTTL != 0 )
            {
                if ( m.mTTL < 0 ) m.mTTL = -1;        // unlimited lives, so to say; persistent packets
                m.mTimeSent = mCurrentTime.asMilliseconds();
                mWaitingForAck[m.mTimeSent] = m;
            }
            ++mSentThisSecond;
        }
    }
        
    // Finally, send ordinary messages, at most one at a time
        
    if ( !mOutGoingMessages.empty() && mSentThisSecond < xMaxMessageCount )
    {
        mCurrentTime = mClock.getElapsedTime();
        cMessage m = mOutGoingMessages.front();
        
        mOutGoingMessages.pop();
        m.mTimeSent = mCurrentTime.asMilliseconds();
        mSocket.send(m.mPacket, mServerAddr, mServerPort);
        
        mSent[m.mSequence] = m;
        
        // This assumes that no two messages will share mCurrentTime
        // If TTL == 0, then we don't wait for ack!
        
        if ( m.mTTL != 0 )
        {
            mWaitingForAck[mCurrentTime.asMilliseconds()] = m;
        }
        
        ++mSentThisSecond;
    }
}
