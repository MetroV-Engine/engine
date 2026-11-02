// The only file that knows the Treasure component.
//
// It defines the types and their E_REFLECT, and nothing else: no header, no
// function, nothing referenced from elsewhere. TestUnknownType.cpp never sees
// these types; it only knows the name "Treasure". Being linked into the test
// executable is enough for the automatic registration to run before main.

#include <string>
#include <vector>

#include "ECS/Reflection/Reflection.hpp"

namespace {
    struct TreasureSpot { float x = 0.0f; float y = 0.0f; };

    struct Treasure {
        std::string name;
        int gold = 0;
        std::vector<std::string> gems;
        TreasureSpot spot;
    };
}

E_REFLECT(TreasureSpot)
    E_FIELD(x, float)
    E_FIELD(y, float)
E_END

E_REFLECT(Treasure)
    E_FIELD(name, std::string)
    E_FIELD(gold, int)
    E_FIELD(gems, std::vector<std::string>)
    E_FIELD(spot, TreasureSpot)
E_END
