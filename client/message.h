#ifndef __clever_client_2__message__
#define __clever_client_2__message__

#include <SFML/Network.hpp>

enum class MSG : sf::Uint16 { firstContact, handshake, ready, unready, lobbyInfo,
    disconnect, syncClocks, spawn, event, worldState };

struct cMessage {
    sf::Packet          mPacket;
    sf::IpAddress       mAddr;
    sf::Uint32          mSequence { 0 };
    sf::Int32           mTimeSent { 0 };
    sf::Int32           mFirstDelta { 0 };
    sf::Int8            mTTL { 0 };
};

#endif /* defined(__clever_client_2__message__) */
