#include "server.h"
#include "helpers.h"
#include <iostream>

sf::Packet& operator>>(sf::Packet& p, sf::Vector2f& v)
{
    return p >> v.x >> v.y;
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

sf::Packet& operator>>(sf::Packet& p, cEntityStatus& e)
{
    p >> e.mOwner >> e.mUid >> e.mCurrentInputState >> e.mLastUpdated;
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

sf::Packet& operator<<(sf::Packet& p, cInputState& is)
{
    return p << is.mLeft << is.mRight << is.mUp << is.mPunch << is.mBlock;
}

sf::Packet& operator<<(sf::Packet& p, sf::Vector2f& v)
{
    return p << v.x << v.y;
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
    {
        p << i;
    }
    
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

cServer::cServer(sf::Uint16 pid, sf::Uint16 expectedclients, sf::Uint16 port, cSceneContainer* scene):
mProtocolID { pid },
mPort { port },
mExpectedClients { expectedclients },
pScene { scene }
{
    if (mSocket.bindPort(mPort) != true )
    {
        throw std::runtime_error("Couldn't bind socket to port.");
    }
    
    pCurrentWorldState = &(pScene->mCurrentWorldState);
    pPreviousWorldState = &(pScene->mPreviousWorldState);
    
    mSocket.setAvgPing(60);
    mSocket.setJitter(10);
    mSocket.setPacketLoss(25);
    
    std::cout << "Server ready and waiting, bound to port " << mPort << ".\n";
    
    mWindow.create(sf::VideoMode(320, 240), "Server view");
    mWindow.setVerticalSyncEnabled(true);
    mWindow.clear();
    
    mView.setSize(640, 480);
    mView.setCenter(320,240);
    mView.setViewport(sf::FloatRect(0,0,1,1));
    
    mWindow.setView(mView);
    
}
    
bool cServer::rogerThat()
{
    bool discard { false };
    
    // Set up ackfield, and check if we can discard the sequence
    // on account of it being too old or already seen
    
    cClientInfo&    client { mClient[inAddr] };
    
    auto previous = client.mSequence;
    if ( later(inSEQ, previous) )
    {
        // If new message in: set up ack bitfield by shifting it to the left
        // a necessary # of places, then flip the bit in position [difference-1] to
        // 1, making it clear that we did receive the message before this one.
        // (The ackfield doesn't contain a sign bit for the LAST acked message.)
        
        auto shift = inSEQ - previous;
        client.mAcks <<= shift;
        client.mAcks[shift-1] = 1;
        client.mSequence = inSEQ;
    }
    else if ( inSEQ != previous )
    {
        // In this case, the incoming packet is not newer than the latest
        // we received, but it's as yet unclear just how old it is.
        
        int shift = previous - inSEQ;
        if ( shift - 1 >= 0 && shift - 1 < 32 )
        {
            // The packet is reasonably old - i.e. the sign bit for it is still
            // in the ackfield
            
            if ( client.mAcks[shift-1] == 0 )
            {
                // this means we haven't acked it yet, so although this is an old
                // packet, this is actually the first time we see it now
                
                client.mAcks[shift-1] = 1;
            }
            else
            {
                // this is an old packet, and we already did see it, so we have
                // to discard it.
                discard = true;
            }
            
        }
        else
        {
            // The packet is too old, we have to discard it.
            discard = true;
        }
    }
    else
    {
        // If we're here, then the incoming sequence is exactly the same
        // as the previous one, which we already acked, so we can discard this one
        discard = true;
    }
    
    return discard;
}
    
void cServer::incomingEvents()
{
    // We assume the incoming message to be in inPacket.
    // First, handle events: input changes
    
    sf::Uint16  eventCount;
    inPacket >> eventCount;
        
    cEvent event;
        
    for( ; eventCount > 0; --eventCount)
    {
        inPacket >> event;
        sync(event.mTime, event.mOwner);        // sync each event and add them
        mHistory.add(event);                    // to history yay
    }
}
    
void cServer::incomingFirstContact()
{
    std::string clientName;
    inPacket >> clientName;
    
    if ( mClient.exist(inAddr) )    // existing client tries to reconnect
    {
        // If there are outstanding handshakes, we should just wait; client
        // may receive those later.
        
        bool waiting { false };
        for (const auto& i : mWaitingForAck)
            if ( i.second.mAddr == inAddr ) waiting = true;
        
        if ( waiting )
        {
            std::cout << "Existing client trying to reconnect, but still outstanding msgs.\n";
            return;
        }
        
        // Otherwise - let's just reset things.
        std::cout << "Existing client reconnects; reset.\n";
        mClient.reset(inAddr, inPort, clientName);
        
    }
    else    // new client
    {
        std::cout << "New client from address " << inAddr.toString() << "; port: " << inPort << "\n";
        mClient.add(inAddr, inPort, clientName);
    }
    
    // Now that the client is created, we can actually "roger" the first contact message
    // (set ackfield appropriately)
    
    rogerThat();
    
    // As a reply to any First Contact msg, we start sending handshake messages,
    // with a TTL of 3.
    
    packHeader(outMessage, mClient[inAddr].mUid, MSG::handshake, mClock.getElapsedTime().asMilliseconds(), 3);
    mUrgentMessages.push(outMessage);
}
    
void cServer::incomingHandshake()
{
    // An incoming handshake message has no payload,
    // but it helps figuring out the RTT.
    
    cClientInfo& client { mClient[inAddr] };
    
    // If inRTT is suspiciously high, drop it, and send new handshake.
    
    if ( inRTT >= xSuspiciousRTT )
    {
        std::cout << "Suspiciously high RTT - chances are that packet was lost and re-sent.\n";
        --client.mHandShaken;
    }
    else
    {
        client.mRTT += inRTT;
    }
    
    
    if ( ++client.mHandShaken == xHandshakeNeeded )
    {
        client.mRTT /= xHandshakeNeeded;
        
        client.mConnected = true;
        std::cout << "Client connected, RTT: " << client.mRTT << "\n";
        
        broadcastLobbyInfo();   // who's here, what's their ping jne.
        
    }
    else    // send the next handshake
    {
        packHeader(outMessage, client.mUid, MSG::handshake, mClock.getElapsedTime().asMilliseconds(), 3);
        mUrgentMessages.push(outMessage);
    }
}
    
void cServer::broadcastLobbyInfo()
{
    std::vector<sf::Uint16> connectedClients;
    
    for ( const auto& i : mClient.mUids )
        if ( mClient[i].mConnected )
        {
            connectedClients.push_back(i);
        }
 
    sf::Int32 time { mClock.getElapsedTime().asMilliseconds() };
    
    for ( const auto& i : mClient.mUids )
    {
        if ( mClient[i].mConnected )
        {
            packHeader(outMessage, i, MSG::lobbyInfo, time, 3);
            outMessage.mPacket << static_cast<sf::Uint16>(connectedClients.size());
            for ( const auto& j : connectedClients )
            {
                outMessage.mPacket << mClient[j].mName;
                outMessage.mPacket << mClient[j].mRTT;
                outMessage.mPacket << mClient[j].mReady;
            }
            mUrgentMessages.push(outMessage);
        }
    }
}
    
void cServer::syncTime()
{
    // Send a "sync clocks NOW" command to everyone, and restart clock
    
    sf::Int32 time { mClock.getElapsedTime().asMilliseconds() };
    
    for ( sf::Uint16 i = 1; i <= mClient.mCount; ++i )
    {
        packHeader(outMessage, i, MSG::syncClocks, time, 10);
        mUrgentMessages.push(outMessage);
    }
    
    mClock.restart();

}
    
void cServer::incomingSync()
{
    // Incoming sync, yay!
    
    mClient[inAddr].mSynced = true;

    if ( mClient.isEveryoneSynced() && !mEveryoneSynced )
    {
        std::cout << "Yay, everyone synced!\n";
        mEveryoneSynced = true;
        mLastTickTime = mClock.getElapsedTime();
        setupStage();
    }
}
    
void cServer::setupStage()
{
    for ( const auto& i : mClient.mUids )
    {
        std::cout << "Adding entity for uid: " << i << "\n";
        sf::Color col;
        switch ( i % 4 ) {
            case 0: col = sf::Color::Blue; break;
            case 1: col = sf::Color::Yellow; break;
            case 2: col = sf::Color::Green; break;
            case 3: col = sf::Color::Red; break;
        }

        mClient[i].mEntityID = pScene->add(i, col);
        mClient[i].mLastAgreedStatus.mUid = mClient[i].mEntityID;
        mClient[i].mLastAgreedStatus.mOwner = mClient[i].mEntityID;
        mISC.mIscCount[i] = 0;      // setting up input change records
    }
}
    
void cServer::incomingReady()
{
    mClient[inAddr].mReady = true;

    // Send ack:
    broadcastLobbyInfo();
    
    // If everyone ready, let's sync clocks:
    
    if ( mClient.isEveryoneReady() && mClient.mCount == mExpectedClients && !mSyncAttempted )
    {
        std::cout << "Everyone ready, syncing clocks...\n";
        syncTime();
        mSyncAttempted = true;
    }
}
    
void cServer::incomingUnready()
{
    mClient[inAddr].mReady = false;
    
    // Send ack:
    broadcastLobbyInfo();
}
    
bool cServer::receiveStuff()
{
    bool    recvd { false };
    
    if ( mSocket.recv(inPacket, inAddr, inPort) )
    {
        bool discard { false };
        
        // Incoming packet header should ALWAYS be:
        // [ protocol ID ] [ client ID ] [ remote sequence ] [ ack ] [ message type ]
        
        inPacket >> inPID >> inUID >> inSEQ >> inACK >> inMSG;
        
        if ( inPID == mProtocolID )
        {
            if ( inMSG != MSG::firstContact )
            {
                discard = rogerThat();
            }
            
            if ( !discard )
            {
                recvd = true;
                
                // If this is not the first contact, then let's adjust
                // our own ack information
                
                if ( inMSG != MSG::firstContact )
                {
                    mClient[inAddr].mLastAck = inACK;
                }
                
                if ( mSent.find(inACK) != mSent.end() && inMSG == MSG::handshake )
                {
                    inRTT = mClock.getElapsedTime().asMilliseconds() - mSent[inACK].mTimeSent;
                }
                
                // Remove acked message from sent and waitingForAck queues
                
                mWaitingForAck.erase(mSent[inACK].mTimeSent);
                mSent.erase(inACK);
                
                // Dispatch, depending on message type.
                
                switch ( inMSG )
                {
                    case MSG::event: {
                            incomingEvents();
                            break;
                    }
                    case MSG::firstContact: {
                        incomingFirstContact();
                        break;
                    }
                    case MSG::handshake: {
                        incomingHandshake();
                        break;
                    }
                    case MSG::ready: {
                        incomingReady();
                        break;
                    }
                    case MSG::unready: {
                        incomingUnready();
                        break;
                    }
                    case MSG::syncClocks: {
                        incomingSync();
                        break;
                    }
                    default:
                        break;
                }
                
            }
                
        }
    }
        
    return recvd;
}
    
void cServer::run()
{
    bool cnt = true;
    while ( cnt )
    {
        if ( sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) )
            cnt = false;
            
        if ( receiveStuff() )
        {
            // Adjust client's estimated RTT
        }
        
        mCurrentTime = mClock.getElapsedTime();
        
        if ( mEveryoneSynced )
        {
            while ( mCurrentTime - mLastTickTime >= xTickTime )
            {
                mLastTickTime += xTickTime;
                tick(mLastTickTime.asMilliseconds());
                sendStuff();
            }
        }
        else    // client not yet synced
        {
            sendStuff();
        }
    }
        
    // Cleanup.
        
    if ( mCurrentTime - mLastSentCleanup >= xSentTimeOut )
    {
        mLastSentCleanup = mCurrentTime;
        refreshSent();
    }
        
    if ( mCurrentTime - mLastHistoryCleanup >= xHistoryTimeOut )
    {
        mLastHistoryCleanup = mCurrentTime;
        mHistory.cleanup((mCurrentTime-xHistoryLength).asMilliseconds());
    }
        
    if ( mCurrentTime - mLastWSACleanup >= xWSATimeOut )
    {
        mLastWSACleanup = mCurrentTime;
        pScene->cleanup((mCurrentTime-xWSALength).asMilliseconds());
    }
    
    sf::sleep(mPause);
}
    
void cServer::refreshSent()
{
    std::vector<sf::Uint32> marked;
    auto end = mWaitingForAck.end();
    
    mCurrentTime = mClock.getElapsedTime();
    
    // Remove those items from the sent queue which:
    // - are not explicitly waiting for an ack, and
    // - which have been on the sent queue for more than xSentTimeout.
    // Well, fair enough, sent is not really a queue but a map. Sue me.
    
    for ( const auto& i : mSent )
    {
        if ( mWaitingForAck.find(i.second.mTimeSent) == end &&
            mCurrentTime.asMilliseconds() - i.second.mTimeSent > xSentTimeOut.asMilliseconds() )
            marked.push_back(i.first);
    }
        
    for ( const auto& i : marked )
        mSent.erase(i);
}
    
void cServer::sendStuff()
{
    while ( !mUrgentMessages.empty() )
    {
        mCurrentTime = mClock.getElapsedTime();
        outMessage = mUrgentMessages.front();
        mUrgentMessages.pop();
        outMessage.mTimeSent = mCurrentTime.asMilliseconds();
        
        mSocket.send(outMessage.mPacket, outMessage.mAddr, mClient[outMessage.mAddr].mPort);
        
        mSent[outMessage.mSequence] = outMessage;
        if ( outMessage.mTTL != 0 )
        {
            mWaitingForAck[mCurrentTime.asMilliseconds()] = outMessage;
        }
    }
        
    mOutboxTimer += mOutboxClock.restart();
    if ( mOutboxTimer >= xOneSec )
    {
        mSentThisSecond = 0;
        mOutboxTimer -= xOneSec;
    }
        
    // OK, so first: packet resend, if any unacked packets that have been waiting for
    // a long time. But in every iteration of sendStuff(), we only resend 1 packet.
    
    if ( !mWaitingForAck.empty() && mSentThisSecond < xMaxMessageCount )
    {
        mCurrentTime = mClock.getElapsedTime();
        if ( mCurrentTime.asMilliseconds() - (mWaitingForAck.begin())->first >= xResendTime.asMilliseconds() )
        {
            // Looks like a packet (or packets) here was lost. So let's resend it.
            // First, remove message from waitinforack, and modify Sent so that we
            // keep track of sending time:
            
            cMessage m = mWaitingForAck.begin()->second;
            
            mWaitingForAck.erase(mWaitingForAck.begin());
            mSent[m.mSequence].mTimeSent = mCurrentTime.asMilliseconds();
            
            mSocket.send(m.mPacket, m.mAddr, mClient[m.mAddr].mPort);
            
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
        mSocket.send(m.mPacket, m.mAddr, mClient[m.mAddr].mPort);
        
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
    
void cServer::moveEntity(const cEvent& event)
{
    // This DOESN'T update the entityinfos, only the entities themselves.
    // EntityInfos get updated at frame and tick time.
    
    pScene->moveEntity(mClient[event.mOwner].mEntityID, event.mInput, event.mTime);
}
    
void cServer::broadcastWorldState(sf::Int32 startOfLastTick)
{
    for (const auto& i : mClient.mUids )
    {
        packHeader(outMessage, i, MSG::worldState, pCurrentWorldState->mTime, 0);
        outMessage.mPacket << *pPreviousWorldState;
        outMessage.mPacket << *pCurrentWorldState;
 
        // Now pack input changes this tick: every player receives the input changes
        // of OTHERS, but not his/her own
        
        auto tmp = mISC.changesForOthers(i);
        outMessage.mPacket << tmp;
        
        for ( auto j = 0; j < mISC.mItemCount; ++j )
        {
            if ( mISC.mItem[j].mOwner != i )
            {
                outMessage.mPacket << mISC.mItem[j];
            }
        }
        
        outMessage.mPacket << mClient[i].mLastAgreedStatus;
        outMessage.mPacket << mClient[i].mDeltaToFirstTick;
        
        
        // Then add hit events. Policy for now: send everything to everyone,
        // and they will sort things out themselves.
        
        outMessage.mPacket << static_cast<sf::Uint16>(pScene->mHitQueue.size());
        
        for ( auto& j : pScene->mHitQueue )
        {
            // Temporarily modify hitEvent in hitQueue, so that time is expressed
            // in delta from start of previous tick. Then we need to switch it back
            // to actual, absolute time, b/c in the next iteration that's how we'll
            // be able to decide whether that particular event is relevant for the
            // next client.
            
            auto tmp = j.mTime;
            
            j.mDeltaSinceLAC = j.mTime - mClient[i].mLastAgreedStatus.mLastUpdated;
            
            j.mTime -= startOfLastTick;
            outMessage.mPacket << j;
            j.mTime = tmp;
        }
        
        // Now push _previous hit queue_ onto the message (redundant; but what if the
        // previous world broadcast state was lost?)
        
        outMessage.mPacket << static_cast<sf::Uint16>(mPreviousHitQueue.size());
        
        for ( auto& j : mPreviousHitQueue )
        {
            auto tmp = j.mTime;
            
            // As above: express hit events with delta from start of tick before the
            // last one
            
            j.mDeltaSinceLAC = j.mTime - mClient[i].mLastAgreedStatus.mLastUpdated;
            j.mTime -= ( startOfLastTick - xTickTime.asMilliseconds() );
            outMessage.mPacket << j;
            j.mTime = tmp;
        }
        
        mUrgentMessages.push(outMessage);
    }
    
    // OK, now saving hit events for own history
    
    for ( auto& j : pScene->mHitQueue )
    {
        mHistory.add(j);
    }
    
    // Archive hit events:
    
    mPreviousHitQueue = pScene->mHitQueue;
    
    // No worries, the hit queue is cleared at the beginning of the each tick.
    
}
    
void cServer::tick(sf::Int32 currentTime)
{

    // Reset input changes
    
    mISC.mItemCount = 0;
    for ( auto& i : mISC.mIscCount )
    {
        i.second = 0;
    }
    
    for ( auto& i : mClient.mUids )
    {
        mClient[i].mFirstDeltaSet = true;
    }
    
    // Clear hit Q
    
    pScene->mHitQueue.clear();
    
    // Rewind time - if we need to.
    
    if ( mHistory.mOldestEventThisTick < pCurrentWorldState->mTime )
    {
        pScene->rewindWorldTo(mHistory.mOldestEventThisTick);
    }
    
    // Get the first event we have to deal with this tick, and the next frame time.
    // The -1 here simply allows for == in the mHistory upper_bound() check.
        
    sf::Int32   updatePtr { pCurrentWorldState->mTime };
    cEvent      nextEvent { mHistory.getNextEvent(updatePtr - 1) };
    sf::Int32   nextFrameTime { updatePtr + xFrameTimeInMs };
    sf::Int32   lastFrameTime { 0 };
    bool        firstWSpassed { false };
    
    sf::Int32   startOfLastTick { currentTime - xTickTime.asMilliseconds() };
    
    // Iterate through history until we reach the present

    while ( nextFrameTime <= currentTime ||
            (nextEvent.mType != Evt::nullevent && nextEvent.mTime <= currentTime ) )
    {
        if ( nextEvent.mType == Evt::nullevent || nextFrameTime < nextEvent.mTime )
        {
                
            // In this case, there are no events to deal with before reaching the
            // next frame, so we just advance the clock and update the world.
            
            pScene->updateWorldAt(nextFrameTime);
            lastFrameTime = nextFrameTime;
            nextFrameTime += xFrameTimeInMs;
        }
        else
        {
            
            // There's an event we have to deal with before the end of this frame.
            // If this event is user-sent, then we need to check if it's a command
            // that came later than the event we currently hold as latest.
                
            if ( nextEvent.mType != Evt::stateRefresh &&
                 nextEvent.mTime > mClient[nextEvent.mOwner].mLastSeenCommandTime )
            {
                mClient[nextEvent.mOwner].mLastSeenCommandTime = nextEvent.mTime;
                mClient[nextEvent.mOwner].mFirstDeltaSet = false;
            }
            
            // Also, if this event is in the latest tick, we'll need to broadcast it
            // to clients, so let's save it to mISC (mInputStateChanges)
            
            if ( nextEvent.mType == Evt::move )
            {
                if ( nextEvent.mTime > startOfLastTick )
                {
                    if ( mISC.mItemCount < mISC.mMaxItems )
                    {
                        mISC.mItem[mISC.mItemCount].mInput = nextEvent.mInput;
                        mISC.mItem[mISC.mItemCount].mDelta = nextEvent.mTime - startOfLastTick;
                        mISC.mItem[mISC.mItemCount].mOwner = nextEvent.mOwner;
                        mISC.mItem[mISC.mItemCount].mEntityID = nextEvent.mEntityID;
                        ++mISC.mIscCount[nextEvent.mOwner];
                        ++mISC.mItemCount;
                    }
                }
                else
                {
                   // Event in the past :(
                }
            }
                
            // Event dispatch
                
            switch ( nextEvent.mType )
            {
                case Evt::move: {
                    
                    moveEntity(nextEvent);
                    break;
                    
                }
                    
                case Evt::hit: {
                    
                    // So this is where we're re-enacting a hit already handled
                    pScene->hitEntity(nextEvent);
                    break;
                    
                }
                
                case Evt::stateRefresh: {
                    
                    // Update a past world state - because something happened
                    // BEFORE. Ignore this step if this is the first (zeroth)
                    // WS we deal with.
                        
                    if ( firstWSpassed )
                    {
                            
                        // At this particular frame, there's a "world status event", meaning
                        // that this is the point where we ticked, and produced a world state
                        // that was then sent out to clients. So once again we need to do
                        // an update, and refresh the world state.
                        
                        pScene->updateWorldAt(nextEvent.mTime);
                        pScene->updateArchiveAt(nextEvent.mTime);
                                                    
                        // If we found a new last command in this particular tick, then
                        // update last-agreed-upon-state for client. It can also be that the tick
                        // yielded no LastSeenCommand, but this happens to be the first tick
                        // in which the currently known last seen command was registered;
                        // in which case we have to update the last-agreed-upon-status.
                        
                        // Sadly we have to do this with all the clients. Event is a simple state
                        // refresh, so it won't have an owner!
                        
                        for ( const auto& i : mClient.mUids )
                        {
                            if ( !mClient[i].mFirstDeltaSet )
                            {
                                mClient[i].mFirstDeltaSet = true;
                                mClient[i].mLastAgreedStatus = pScene->getStatus(i);
                                mClient[i].mDeltaToFirstTick = nextEvent.mTime - mClient[i].mLastSeenCommandTime;
                            }
                            else
                                if ( mClient[i].mLastAgreedStatus.mLastUpdated == nextEvent.mTime )
                                {
                                    mClient[i].mLastAgreedStatus = pScene->getStatus(i);
                                }
                        }
                        
                    }
                    else
                    {
                        firstWSpassed = true;
                    }
                    break;
                }
                default:
                    break;
            }
                
            updatePtr = nextEvent.mTime;
            nextEvent = mHistory.getNextEvent(updatePtr);
        }
    }
        
    // Now we have to deal with leftover time, in case the tick
    // doesn't end exactly at tick time - which is quite likely to happen -
        
    if ( lastFrameTime < currentTime )
    {
        pScene->updateWorldAt(currentTime);
    }
        
    // Calculate client's last-command deltas and last agreed status (if necessary)
    
    for ( const auto& i : mClient.mUids )
    {
        if ( !mClient[i].mFirstDeltaSet )
        {
            mClient[i].mFirstDeltaSet = true;
            mClient[i].mLastAgreedStatus = pScene->getStatus(i);
            mClient[i].mDeltaToFirstTick = currentTime - mClient[i].mLastSeenCommandTime;
        }
    }

    // Finally, add our own event, and reset history's mOldestEventThisTick,
    // in anticipation of the next one. Events in that tick will be compared to
    // this WS-event.
        
    cEvent event { Evt::stateRefresh };
    event.mTime = currentTime;
    mHistory.add(event);
    mHistory.resetOldestEvent();
    
    pScene->updatePrevWorldState();

    // Save current state
    
    pScene->saveCurrent(currentTime);
    
    // Finally, broadcast event (and this is where rendering would come too)
        
    broadcastWorldState(startOfLastTick);
        
    mWindow.clear();
    pScene->render(mWindow);
    mWindow.display();

}
    
inline void cServer::sync(sf::Int32& time, sf::Uint16 id)
{
    time += mClient[id].mRTT;
    
    auto timeNow = mClock.getElapsedTime().asMilliseconds();
    if ( time > timeNow )
    {
        // This would mean that we're overestimated RTT, and trying to add
        // event into future... that's inadmissibble! Here, RTT could be
        // adjusted to prevent such things happening in the future. For now,
        // just clamp down on time and make sure that event will actually be
        // processed in this tick.
        
        time = timeNow;
    }
}
    
void cServer::packHeader(cMessage& msg, sf::Uint16 uid, MSG mtype, sf::Int32 time, sf::Uint16 ttl)
{
    msg.mPacket.clear();
    msg.mPacket << mProtocolID << uid << mLocalSequence << mClient[uid].mSequence << mClient[uid].mAcks << mtype;
    msg.mAddr = mClient[uid].mAddr;
    msg.mTimeSent = time;
    msg.mTTL = ttl;
    msg.mSequence = mLocalSequence;
    ++mLocalSequence;
}