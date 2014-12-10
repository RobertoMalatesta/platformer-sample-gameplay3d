#include "PlayerHandOfGodComponent.h"

#include "CollisionObjectComponent.h"
#include "Common.h"
#include "GameObject.h"
#include "LevelComponent.h"
#include "Messages.h"
#include "MessagesPlatformer.h"
#include "PlayerComponent.h"

namespace platformer
{
    PlayerHandOfGodComponent::PlayerHandOfGodComponent()
        : _boundaryScale(gameplay::Vector2::one())
        , _levelLoaded(false)
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
        _splashScreenFadeMessage = PlatformerSplashScreenChangeRequestMessage::create();
    }

    void PlayerHandOfGodComponent::finalize()
    {
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_splashScreenFadeMessage);
        SAFE_RELEASE(_player);
    }

    void PlayerHandOfGodComponent::update(float)
    {
        if(_levelLoaded && (_forceNextReset || !_boundary.intersects(_player->getPosition().x, _player->getPosition().y, 1, 1)))
        {
            _forceNextReset = false;

            if (gameplay::Node * node = _player->getCharacterNode())
            {
                gameplay::PhysicsCharacter * character = node->getCollisionObject()->asCharacter();
                float const fadeInDuration = 0.0f;
                PlatformerSplashScreenChangeRequestMessage::setMessage(_splashScreenFadeMessage, fadeInDuration, true);
                getRootParent()->broadcastMessage(_splashScreenFadeMessage);
                float const fadeOutDuration = 1.0f;
                PlatformerSplashScreenChangeRequestMessage::setMessage(_splashScreenFadeMessage, fadeOutDuration, false);
                getRootParent()->broadcastMessage(_splashScreenFadeMessage);
                node->setTranslation(_resetPosition.x, _resetPosition.y, 0);
            }
        }
    }

    void PlayerHandOfGodComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        switch (message->getId())
        {
        case(Messages::Type::LevelLoaded):
        {
            LevelComponent * level = getRootParent()->getComponentInChildren<LevelComponent>();
            _resetPosition = level->getPlayerSpawnPosition();
            float const width = ((level->getWidth() + 1) * level->getTileWidth()) * PLATFORMER_UNIT_SCALAR;
            float const height = ((level->getHeight() + 1) * level->getTileHeight()) * PLATFORMER_UNIT_SCALAR;
            _boundary.width = width * _boundaryScale.x;
            _boundary.height = height * _boundaryScale.y;
            _boundary.x = (width - _boundary.width) / 2;
            _boundary.y = -height + (height - _boundary.height) / 2;
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

    void PlayerHandOfGodComponent::readProperties(gameplay::Properties & properties)
    {
        properties.getVector2("level_boundary_scale", &_boundaryScale);
    }
}
