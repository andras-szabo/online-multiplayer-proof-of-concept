#ifndef __clever_server_3__helpers__
#define __clever_server_3__helpers__

#include <iostream>
#include <SFML/System.hpp>

const unsigned int gMax { std::numeric_limits<sf::Uint32>::max() / 2 };

inline bool later(sf::Uint32 a, sf::Uint32 b)
{
    return ( a > b && a - b < gMax ) || ( a < b && b - a > gMax );
}
#endif /* defined(__clever_server_3__helpers__) */
