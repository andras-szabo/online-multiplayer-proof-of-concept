#ifndef __clever_server_3__message__
#define __clever_server_3__message__

#include <SFML/Network.hpp>

enum class MSG : sf::Uint16 { firstContact, handshake, ready, unready, lobbyInfo,
    disconnect, syncClocks, spawn, event, worldState };

struct cMessage {
    sf::Packet          mPacket;
    sf::IpAddress       mAddr;
    sf::Uint32          mSequence;
    sf::Int32           mTimeSent;
    sf::Int8            mTTL;
};

#endif /* defined(__clever_server_3__message__) */
