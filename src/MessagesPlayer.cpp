#include "gameplay.h"
#include "GameObjectController.h"
#include "MessagesPlayer.h"

namespace platformer
{
    gameplay::AIMessage * PlayerJumpMessage::create()
    {
        return gameplay::AIMessage::create(Messages::Type::PlayerJump, "", "", 0);
    }

    gameplay::AIMessage * PlayerForceHandOfGodResetMessage::create()
    {
        return gameplay::AIMessage::create(Messages::Type::PlayerForceHandOfGodReset, "", "", 0);
    }
}
