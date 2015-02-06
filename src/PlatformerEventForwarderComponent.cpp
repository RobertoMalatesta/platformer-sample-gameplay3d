#include "PlatformerEventForwarderComponent.h"

#include "Messages.h"
#include "Platformer.h"

namespace game
{
    PlatformerEventForwarderComponent::PlatformerEventForwarderComponent()
    {
    }

    PlatformerEventForwarderComponent::~PlatformerEventForwarderComponent()
    {
    }

    bool PlatformerEventForwarderComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
        Platformer * game = static_cast<Platformer*>(gameplay::Game::getInstance());

        switch (messageType)
        {
            case (Messages::Type::RequestSplashScreenFade):
            {
                RequestSplashScreenFadeMessage msg(message);
                game->setSplashScreenFade(msg._duration, msg._isFadingIn, msg._showLogo);
            }
            break;
        }

        return true;
    }
}
