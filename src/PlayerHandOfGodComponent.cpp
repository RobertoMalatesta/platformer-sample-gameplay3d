#include "PlayerHandOfGodComponent.h"

#include "CameraControlComponent.h"
#include "Common.h"
#include "GameObject.h"
#include "LevelLoaderComponent.h"
#include "Messages.h"
#include "PlayerComponent.h"
#include "ScreenRenderer.h"

namespace game
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
        _camera = getRootParent()->getComponent<CameraControlComponent>();
        _camera->addRef();
    }

    void PlayerHandOfGodComponent::initialize()
    {
    }

    void PlayerHandOfGodComponent::finalize()
    {
        SAFE_RELEASE(_player);
        SAFE_RELEASE(_camera);
    }

    void PlayerHandOfGodComponent::update(float)
    {
        if(_levelLoaded && (_forceNextReset || !_boundary.intersects(_player->getPosition().x, _player->getPosition().y, 1, 1)))
        {
            _forceNextReset = false;

            if (_player)
            {
                float const fadeOutDuration = 1.15f;
                ScreenRenderer::getInstance().queueFadeToBlack(0.0f);
                ScreenRenderer::getInstance().queueFadeOut(fadeOutDuration);
                _player->reset(_resetPosition);
            }
        }
    }

    bool PlayerHandOfGodComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
        switch (messageType)
        {
        case(Messages::Type::LevelLoaded):
        {
            LevelLoaderComponent * level = getRootParent()->getComponentInChildren<LevelLoaderComponent>();
            _resetPosition = level->getPlayerSpawnPosition();
            float const height = ((level->getHeight() + 1) * level->getTileHeight()) * GAME_UNIT_SCALAR;
            _boundary.width = _camera->getTargetBoundary().width;
            _boundary.height = height;
            _boundary.x = _camera->getTargetBoundary().x;
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

        return true;
    }
}
