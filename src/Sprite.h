#ifndef GAME_SPRITE_H
#define GAME_SPRITE_H

#include "Rectangle.h"
#include <string>

namespace game
{
    /** @script{ignore} */
    struct Sprite
    {
        std::string _name;
        gameplay::Rectangle _src;
        bool _isRotated;
    };
}

#endif
