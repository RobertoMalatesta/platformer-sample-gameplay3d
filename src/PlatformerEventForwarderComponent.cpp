#include "PlatformerEventForwarderComponent.h"

#include "Messages.h"
#include "Platformer.h"

namespace platformer
{
    PlatformerEventForwarderComponent::PlatformerEventForwarderComponent()
    {
    }

    PlatformerEventForwarderComponent::~PlatformerEventForwarderComponent()
    {
    }

    void PlatformerEventForwarderComponent::onMessageReceived(gameobjects::GameObjectMessage message, int messageType)
    {
        Platformer * game = static_cast<Platformer*>(gameplay::Game::getInstance());

        switch (messageType)
        {
            case (Messages::Type::RequestSplashScreenFade):
            {
                RequestSplashScreenFadeMessage msg(message);
                game->setSplashScreenFade(msg._duration, msg._isFadingIn);
            }
            break;
        }
    }
}
