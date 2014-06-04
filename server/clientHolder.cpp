#include "clientHolder.h"
#include <iostream>

cClientInfo& cClientHolder::operator[](sf::IpAddress addr)
{
    return mClientByAddress[addr];
}

cClientInfo& cClientHolder::operator[](sf::Uint16 uid)
{
    return cClientHolder::operator[](mAddress[uid]);
}

bool cClientHolder::exist(sf::IpAddress addr) const
{
    return ( mClientByAddress.find(addr) != mClientByAddress.end() );
}

bool cClientHolder::exist(sf::Uint16 uid) const
{
    return ( mAddress.find(uid) != std::end(mAddress) );
}

void cClientHolder::reset(sf::IpAddress addr, sf::Uint16 port, const std::string& name)
{
    mClientByAddress[addr].reset(addr, port, name);
}

bool cClientHolder::isEveryoneReady()
{
    for ( const auto& i : mClientByAddress )
    {
        if ( i.second.mReady == false )
            return false;
    }
    return true;
}

bool cClientHolder::isEveryoneSynced()
{
    for ( const auto& i : mClientByAddress )
    {
        if ( i.second.mReady == false )
            return false;
    }
    return true;
}

void cClientHolder::add(sf::IpAddress addr,
                        sf::Uint16 port,
                        const std::string& name)
{
    ++mCount;
    
    cClientInfo ci;
    ci.mAddr = addr;
    ci.mPort = port;
    ci.mName = name;
    ci.mUid = mNextAvailableUid;
    
    mClientByAddress[addr] = std::move(ci);
    mAddress[mNextAvailableUid] = addr;
    mUids.push_back(mNextAvailableUid);
    
    ++mNextAvailableUid;
    
    std::cout << "Client added with uid: " << mClientByAddress[addr].mUid << "\n";
}

void cClientHolder::disconnect(sf::Uint16 id)
{
    if ( !exist(id) ) return;
    
    sf::IpAddress addr = mAddress[id];
    mClientByAddress.erase(addr);
    mAddress.erase(id);
    
    mCount = 0;
    mUids.clear();
    for ( const auto& i : mClientByAddress )
    {
        ++mCount;
        mUids.push_back(i.second.mUid);
    }
}