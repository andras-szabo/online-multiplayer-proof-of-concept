#include <iostream>

#include "netsim.h"

const void* customPacket::onSend(std::size_t& size)
{
    // This expects that addressAsUint, port, and packPointer are
    // all set up, such that packPointer points to an area of memory
    // where we can freely play with the packet.
    
    // Address and itmp might need to change endianness.
    addressAsUint = HTONL(addressAsUint);
    itmp = HTONL(itmp);
    
    auto aSize = sizeof(addressAsUint);
    auto tSize = sizeof(itmp);
    
    size = aSize + tSize + originalSize;
    
    if ( size > sf::UdpSocket::MaxDatagramSize )
        throw std::runtime_error("Packet too large!");
    
    // First thing that goes in the packet: address as uint.;
    std::memcpy(packPointer, &addressAsUint, aSize);
    
    // Second thing: simulated waiting time, in milliseconds.
    std::memcpy(packPointer + sizeof(addressAsUint), &itmp, tSize);
    
    // Third thing: data.
    std::memcpy(packPointer + aSize + tSize, originalData, originalSize);
    
    return packPointer;
}

cNetSimSocket::cNetSimSocket():
simAddress { sf::IpAddress::getLocalAddress() },
packetLoss { 0 },
avgPing { 0 },
jitter { 1 },
simPort { 0 }
{
    srand(time(NULL));
    clock.restart();
    
    // Bookkeeping: the simulated address actually translates
    // to the local address.
    
    dir[simAddress.toInteger()] = sf::IpAddress::getLocalAddress();
    
    // Ok here we reserve memory that can surely hold a packet.
    // This is the "staging area" of memory, where the original
    // packet is going to be modified, via insertion of 2
    // additional fields, containing the simulated address
    // and the simulated ping. Destructor will invoke delete.
    
    packPointer = new char[sf::UdpSocket::MaxDatagramSize];
}

cNetSimSocket::cNetSimSocket(sf::IpAddress ip):
simAddress { ip },
packetLoss { 0 },
avgPing { 0 },
jitter { 1 },
simPort { 0 }
{
    srand(time(NULL));
    clock.restart();
    
    // See comments at default ctor.
    
    dir[simAddress.toInteger()] = sf::IpAddress::getLocalAddress();
    packPointer = new char[sf::UdpSocket::MaxDatagramSize];
}

cNetSimSocket::~cNetSimSocket()
{
    delete[] packPointer;
}

void cNetSimSocket::setAddress(sf::IpAddress ip)
{
    simAddress = ip;
    dir[simAddress.toInteger()] = sf::IpAddress::getLocalAddress();
}

bool cNetSimSocket::bindPort(unsigned short port)
{
    if ( port == 0 )
    {
        if ( socket.bind(sf::Socket::AnyPort) != sf::Socket::Done )
            return false;
            // throw std::runtime_error("Can't bind socket to any port.\n");
        simPort = socket.getLocalPort();
    }
    else
    {
        if ( socket.bind(port) != sf::Socket::Done )
            // throw std::runtime_error("Can't bind socket to specific port.\n");
            return false;
        
        simPort = port;
    }
    
    socket.setBlocking(false);
    
    return true;
}

void cNetSimSocket::send(sf::Packet& packet, sf::IpAddress& address, unsigned short port)
{
    if ( (rand() % 99) + 1  > packetLoss )
    {
        customPacket pack;
        pack.packPointer = packPointer;
        pack.addressAsUint = simAddress.toInteger();
        pack.itmp = avgPing + (rand() % jitter) - jitter / 2;
        pack.originalData = packet.getData();
        pack.originalSize = packet.getDataSize();
        
        // Dir[]: the directory, which tells which simulated
        // address equals which actual address. If an address
        // is not in the address book, then we'll simply use
        // the original one.
    
        auto it = dir.find(address.toInteger());
        if ( it == dir.end() )
        {
            socket.send(pack, address, port);
        }
        else
        {
            socket.send(pack, it->second, port);
        }
    }
    else
    {
        std::cout << "Packet lost! - ";
    }
}

bool cNetSimSocket::recv(sf::Packet& packet, sf::IpAddress& address, unsigned short& port)
{
    // Did we actually get new packet? If so, push it on the priority queue,
    // which is ordered in function of waiting times.
    
    currentTime = clock.getElapsedTime();
    
    if ( socket.receive(inPacket, inAddr, inPort) == sf::Socket::Done )
    {
        // First chop off the fields containing simulated address (as Uint)
        // and simulated ping (as Int); then, using these data, insert the
        // entry into the priority queue.
        
        inPacket >> uitmp >> itmp;
        waitingPacket wp { currentTime+sf::milliseconds(itmp), uitmp, inPort, inPacket };
        q.push(std::move(wp));
        
        // Update address book, if needed.
        
        if ( dir.find(uitmp) == dir.end() )
        {
            dir[uitmp] = inAddr;
        }
        
    }
    
    // Check the queue: pop message if its "waiting time" is smaller than
    // the current time (waiting has expired), in which case we'll return that msg.
    
    if ( q.empty() ) return false;
    
    if ( q.top().timeToWait < currentTime )
    {
        packet = q.top().packet;
        address = sf::IpAddress { q.top().addressAsUint };
        port = q.top().port;
        q.pop();
        return true;
    }
    
    // If no messages returned, return false.
    
    return false;
}

