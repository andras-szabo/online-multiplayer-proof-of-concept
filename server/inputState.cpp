#include "inputState.h"

sf::Uint16 cISC::changesForOthers(sf::Uint16 id) const
{
    sf::Uint16 n { 0 };
    for ( const auto& i : mIscCount )
        if ( i.first != id ) n += i.second;

    return n;
}