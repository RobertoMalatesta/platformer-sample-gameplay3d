#include "EnemyComponent.h"

#include "Common.h"
#include "CollisionObjectComponent.h"
#include "GameObject.h"
#include "SpriteAnimationComponent.h"

namespace game
{
    EnemyComponent::EnemyComponent()
        : _isLeftFacing(MATH_RANDOM_0_1() > 0.5f ? true : false)
        , _movementSpeed(5.0f)
        , _triggerNode(nullptr)
        , _state(State::Walking)
        , _minX(std::numeric_limits<float>::min())
        , _maxX(std::numeric_limits<float>::max())
        , _alpha(1.0f)
        , _snapToCollisionY(true)
    {
    }

    EnemyComponent::~EnemyComponent()
    {
    }

    void EnemyComponent::onStart()
    {
        _animations[State::Walking] = getParent()->findComponent<SpriteAnimationComponent>(_walkAnimComponentId);
        _animations[State::Dead] = getParent()->findComponent<SpriteAnimationComponent>(_deathAnimComponentId);

        _triggerNode = getParent()->findComponent<CollisionObjectComponent>(_triggerComponentId)->getNode();
        _triggerNode->addRef();

        getCurrentAnimation()->play();
    }

    void EnemyComponent::finalize()
    {
        SAFE_RELEASE(_triggerNode);
    }

    void EnemyComponent::readProperties(gameplay::Properties & properties)
    {
        _walkAnimComponentId = properties.getString("walk_anim");
        _deathAnimComponentId = properties.getString("death_anim");
        _triggerComponentId = properties.getString("collision_trigger");
        _movementSpeed = properties.getFloat("speed");

        char const * snapId = "snap_to_collision_y";
        if (properties.exists(snapId))
        {
            _snapToCollisionY = properties.getBool(snapId);
        }
    }

    EnemyComponent::State::Enum EnemyComponent::getState() const
    {
        return _state;
    }

    gameplay::Vector2 EnemyComponent::getPosition() const
    {
        return gameplay::Vector2(_triggerNode->getTranslation().x, _triggerNode->getTranslation().y);
    }

    void EnemyComponent::forEachAnimation(std::function <bool(State::Enum, SpriteAnimationComponent *)> func)
    {
        for (auto & pair : _animations)
        {
            if (func(pair.first, pair.second))
            {
                break;
            }
        }
    }

    void EnemyComponent::onTerrainCollision()
    {
        _isLeftFacing = !_isLeftFacing;
    }

    void EnemyComponent::update(float elapsedTime)
    {
        if(_state != State::Dead)
        {
            // Calculate the next step
            float const dt = elapsedTime / 1000.0f;
            float velocityX = _movementSpeed *(_isLeftFacing ? -1.0f : 1.0f) * dt;
            float originalTranslationX = _triggerNode->getTranslationX();
            float nextTranslationX = originalTranslationX + velocityX;
            float const offset = _triggerNode->getScaleX();
            float clampedTranslationX = MATH_CLAMP(nextTranslationX, _minX + offset, _maxX - offset);

            // If it exceeds the constraints along x then use the clamped translation and turn around
            if(nextTranslationX != clampedTranslationX)
            {
                _isLeftFacing = !_isLeftFacing;
                _triggerNode->setTranslationX(clampedTranslationX);
                velocityX *= -1.0f;
            }

            _triggerNode->translateX(velocityX);
        }
        else if(_alpha > 0.0f)
        {
            float const fadeOutSpeed = 0.5f;
            float const dt = elapsedTime / 1000.0f;
            _alpha = MATH_CLAMP(_alpha - (dt * fadeOutSpeed), 0, 1.0f);
        }
    }

    SpriteAnimationComponent * EnemyComponent::getCurrentAnimation()
    {
        return _animations[_state];
    }

    gameplay::Node * EnemyComponent::getTriggerNode() const
    {
        return _triggerNode;
    }

    bool EnemyComponent::isLeftFacing() const
    {
        return _isLeftFacing;
    }

    float EnemyComponent::getAlpha()
    {
        return _alpha;
    }

    void EnemyComponent::setHorizontalConstraints(float minX, float maxX)
    {
        _minX = minX;
        _maxX = maxX;
    }

    void EnemyComponent::kill()
    {
        _state = State::Dead;
        getCurrentAnimation()->play();
        _triggerNode->getCollisionObject()->setEnabled(false);
    }

    bool EnemyComponent::isSnappedToCollisionY() const
    {
        return _snapToCollisionY;
    }
}
