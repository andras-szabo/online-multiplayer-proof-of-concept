#ifndef __clever_server_4__clientHolder__
#define __clever_server_4__clientHolder__

#include "clientInfo.h"
#include <map>
#include <SFML/System.hpp>
#include <vector>

class cClientHolder {
public:
    bool                exist(sf::IpAddress) const;
    bool                exist(sf::Uint16) const;
    void                reset(sf::IpAddress, sf::Uint16, const std::string&);
    void                add(sf::IpAddress, sf::Uint16, const std::string&);
    bool                isEveryoneReady();
    bool                isEveryoneSynced();
    void                disconnect(sf::Uint16);
    
    cClientInfo&        operator[](sf::IpAddress);
    cClientInfo&        operator[](sf::Uint16);
    
public:
    sf::Uint16                  mCount { 0 };
    std::vector<sf::Uint16>     mUids;
    
private:
    std::map<sf::IpAddress, cClientInfo>    mClientByAddress;
    std::map<sf::Uint16, sf::IpAddress>     mAddress;
    sf::Uint16                              mNextAvailableUid { 1 };
};

#endif /* defined(__clever_server_4__clientHolder__) */
