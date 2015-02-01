#ifndef PLATFORMER_MESSAGES_H
#define PLATFORMER_MESSAGES_H

#include "GameObjectCommon.h"

namespace platformer
{
    GAMEOBJECTS_MESSAGE_TYPES_BEGIN()
        GAMEOBJECTS_MESSAGE_TYPE(Key)
        GAMEOBJECTS_MESSAGE_TYPE(Mouse)
        GAMEOBJECTS_MESSAGE_TYPE(Touch)
        GAMEOBJECTS_MESSAGE_TYPE(Pinch)
        GAMEOBJECTS_MESSAGE_TYPE(Gamepad)
        GAMEOBJECTS_MESSAGE_TYPE(PlayerJump)
        GAMEOBJECTS_MESSAGE_TYPE(PlayerForceHandOfGodReset)
        GAMEOBJECTS_MESSAGE_TYPE(LevelLoaded)
        GAMEOBJECTS_MESSAGE_TYPE(PreLevelUnloaded)
        GAMEOBJECTS_MESSAGE_TYPE(LevelUnloaded)
        GAMEOBJECTS_MESSAGE_TYPE(RequestLevelReload)
        GAMEOBJECTS_MESSAGE_TYPE(RequestSplashScreenFade)
    GAMEOBJECTS_MESSAGE_TYPES_END()

    GAMEOBJECTS_MESSAGE_0(LevelLoaded)
    GAMEOBJECTS_MESSAGE_0(LevelUnloaded)
    GAMEOBJECTS_MESSAGE_0(RequestLevelReload)
    GAMEOBJECTS_MESSAGE_0(PreLevelUnloaded)
    GAMEOBJECTS_MESSAGE_0(PlayerJump)
    GAMEOBJECTS_MESSAGE_0(PlayerForceHandOfGodReset)
    GAMEOBJECTS_MESSAGE_2(Key, int, event, int, key)
    GAMEOBJECTS_MESSAGE_2(Gamepad, int, event, int, gamepadIndex)
    GAMEOBJECTS_MESSAGE_3(RequestSplashScreenFade, float, duration, bool, isFadingIn, bool, showLogo)
    GAMEOBJECTS_MESSAGE_3(Pinch, int, x, int, y, float, scale)
    GAMEOBJECTS_MESSAGE_4(Touch, int, event, int, x, int, y, int, contactIndex)
    GAMEOBJECTS_MESSAGE_4(Mouse, int, event, int, x, int, y, int, wheelDelta)
}

#endif
