#include "PlayerHandOfGodComponent.h"

#include "CollisionObjectComponent.h"
#include "Common.h"
#include "GameObject.h"
#include "LevelComponent.h"
#include "Messages.h"
#include "PlayerComponent.h"

namespace platformer
{
    PlayerHandOfGodComponent::PlayerHandOfGodComponent()
        : _levelLoaded(false)
        , _forceNextReset(false)
    {
    }

    PlayerHandOfGodComponent::~PlayerHandOfGodComponent()
    {
    }

    void PlayerHandOfGodComponent::onStart()
    {
        _player = getParent()->getComponent<PlayerComponent>();
        _player->addRef();
    }

    void PlayerHandOfGodComponent::initialize()
    {
        _splashScreenFadeMessage = RequestSplashScreenFadeMessage::create();
    }

    void PlayerHandOfGodComponent::finalize()
    {
        GAMEOBJECTS_DELETE_MESSAGE(_splashScreenFadeMessage);
        SAFE_RELEASE(_player);
    }

    void PlayerHandOfGodComponent::update(float)
    {
        if(_levelLoaded && (_forceNextReset || !_boundary.intersects(_player->getPosition().x, _player->getPosition().y, 1, 1)))
        {
            _forceNextReset = false;

            if (gameplay::Node * node = _player->getCharacterNode())
            {
                float const fadeInDuration = 0.0f;
                RequestSplashScreenFadeMessage::setMessage(_splashScreenFadeMessage, fadeInDuration, true);
                getRootParent()->broadcastMessage(_splashScreenFadeMessage);
                float const fadeOutDuration = 1.0f;
                RequestSplashScreenFadeMessage::setMessage(_splashScreenFadeMessage, fadeOutDuration, false);
                getRootParent()->broadcastMessage(_splashScreenFadeMessage);
                _player->reset(_resetPosition);
            }
        }
    }

    void PlayerHandOfGodComponent::onMessageReceived(gameobjects::GameObjectMessage message, int messageType)
    {
        switch (messageType)
        {
        case(Messages::Type::LevelLoaded):
        {
            LevelComponent * level = getRootParent()->getComponentInChildren<LevelComponent>();
            _resetPosition = level->getPlayerSpawnPosition();
            float const width = ((level->getWidth() + 1) * level->getTileWidth()) * PLATFORMER_UNIT_SCALAR;
            float const height = ((level->getHeight() + 1) * level->getTileHeight()) * PLATFORMER_UNIT_SCALAR;
            _boundary.width = width;
            _boundary.height = height;
            _boundary.x = (width - _boundary.width) / 2;
            _boundary.y = -height + (height - _boundary.height) / 2;
            _boundary.height = std::numeric_limits<float>::max();
            _levelLoaded = true;
        }
            break;
        case(Messages::Type::LevelUnloaded):
            _levelLoaded = false;
            break;
        case(Messages::Type::PlayerForceHandOfGodReset):
            _forceNextReset = true;
            break;
        }
    }
}
