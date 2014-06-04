//
//  netsim.h
//  netsimulator
//
//  Created by Andras Szabo on 02/04/14.
//  Copyright (c) 2014 Andras Szabo. All rights reserved.
//
//  Example usage:
//	
//	cNetSimSocket socket;
//	cNetSimSocket socket(“1.2.3.4”);	// Simulated address: 1.2.3.4
//	socket.setPacketLoss(10);           // 10% packet loss chance
//	socket.setAvgPing(100);             // avg RTT: 200 ms
//	socket.setJitter(25);               // RTT/2 will vary between 75 and 125
//
//	socket.bindPort();                  // Bind to any available port
//
//	socket.bindPort(54000);             // Bind to specific port;
//
//  To receive:
//	
//	bool success = socket.recv(inPacket, inAddr, inPort);
//	(true if received something)
//
//  To send:
//
//	socket.send(outPacket, outAddr, outPort);
//
//  These special variants of send() and recv() work with simulated addresses;
//  cNetSimSocket does internal bookkeeping to match simulated addresses with
//  real ones.
//
//  Because of the simulation overhead, packets must not be greater than
//  sf::UdpSocket::MaxDatagramSize()-8; i.e. I need 8 additional bytes,
//  to let the server know about the simulated address and the expected ping.


#ifndef __netsimulator__netsim__
#define __netsimulator__netsim__

#include <SFML/Network.hpp>
#include <queue>
#include <map>

struct waitingPacket {

    waitingPacket(sf::Time t, sf::Uint32 a, unsigned short po, sf::Packet& pa):
    timeToWait { t },
    addressAsUint { a },
    port { po },
    packet { pa }
    {
        
    }
    
    bool operator<(const waitingPacket& x) const { return timeToWait > x.timeToWait; }
    
    sf::Time            timeToWait;
    sf::Uint32          addressAsUint;
    unsigned short      port;
    sf::Packet          packet;
};

class customPacket : public sf::Packet {
    virtual const void* onSend(std::size_t& size);

public:
    sf::Uint32          addressAsUint;
    sf::Int32           itmp;
    char*               packPointer;
    const void*         originalData;
    size_t              originalSize;
};

class cNetSimSocket {
public:
    cNetSimSocket();
    cNetSimSocket(sf::IpAddress);
    ~cNetSimSocket();
    
    bool        bindPort(unsigned short = 0);
    
    void        setAddress(sf::IpAddress ip);
    void        setPacketLoss(int p) { packetLoss = p; }
    void        setAvgPing(int a) { avgPing = a; }
    void        setJitter(int j) { jitter = j; if ( jitter == 0 ) jitter = 1; }
    
                // Jitter must not be 0 - otherwise it would throw when
                // generating random number (rand() % jitter).
    
    void        send(sf::Packet&, sf::IpAddress&, unsigned short);
    bool        recv(sf::Packet&, sf::IpAddress&, unsigned short&);

    unsigned short  getPort() const { return simPort; }
    
    char*       packPointer;        // Will point to data that actually
                                    // needs to be sent (we're inserting
                                    // additional data into the beginning of
                                    // the packet).
    
private:
    std::priority_queue<waitingPacket>      q;
    std::map<sf::Uint32, sf::IpAddress>     dir;
    
    sf::UdpSocket       socket;
    sf::IpAddress       simAddress; // The simulated IP address
    int                 packetLoss; // Probability of packet loss, %
    int                 avgPing;    // Simulated avg. RTT/2, in milliseconds
    int                 jitter;     // Interval into which ping will fall
    unsigned short      simPort;    // Simulated port
    
    sf::Uint32          uitmp;
    int                 itmp;
    
    sf::Packet          inPacket;
    sf::IpAddress       inAddr;
    unsigned short      inPort;
    
    sf::Clock           clock;
    sf::Time            currentTime;
};

#endif /* defined(__netsimulator__netsim__) */
