#include "PlayerComponent.h"

#include "CameraControlComponent.h"
#include "Common.h"
#include "CollisionObjectComponent.h"
#include "GameObject.h"
#include "MessagesPlayer.h"
#include "SpriteAnimationComponent.h"

namespace platformer
{
    PlayerComponent::PlayerComponent()
        : _isLeftFacing(false)
        , _movementDirection(MovementDirection::None)
        , _previousState(State::Idle)
        , _movementSpeed(5.0f)
        , _jumpHeight(1.0f)
        , _characterNode(nullptr)
        , _characterNormalNode(nullptr)
        , _movementScale(0.0f)
        , _jumpMessage(nullptr)
        , _climbingEnabled(false)
        , _climbingSnapPositionX(0.0f)
    {
    }

    PlayerComponent::~PlayerComponent()
    {
    }

    void PlayerComponent::onStart()
    {
        _animations[State::Idle] = getParent()->findComponent<SpriteAnimationComponent>(_idleAnimComponentId);
        _animations[State::Walking] = getParent()->findComponent<SpriteAnimationComponent>(_walkAnimComponentId);
        _animations[State::Cowering] = getParent()->findComponent<SpriteAnimationComponent>(_cowerAnimComponentId);
        _animations[State::Jumping] = getParent()->findComponent<SpriteAnimationComponent>(_jumpAnimComponentId);
        _animations[State::Climbing] = getParent()->findComponent<SpriteAnimationComponent>(_climbingCharacterComponentId);

        _characterNormalNode = getParent()->findComponent<CollisionObjectComponent>(_normalCharacterComponentId)->getNode();
        _characterNormalNode->addRef();
        _characterNode = _characterNormalNode;
        _jumpMessage = PlayerJumpMessage::create();
        _state = State::Idle;
        _camera = getRootParent()->getComponent<CameraControlComponent>();
        _camera->addRef();
    }

    void PlayerComponent::finalize()
    {
        _characterNode = nullptr;
        SAFE_RELEASE(_camera);
        SAFE_RELEASE(_characterNormalNode);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_jumpMessage);
    }

    void PlayerComponent::readProperties(gameplay::Properties & properties)
    {
        _idleAnimComponentId = properties.getString("idle_anim");
        _walkAnimComponentId = properties.getString("walk_anim");
        _cowerAnimComponentId = properties.getString("cower_anim");
        _jumpAnimComponentId = properties.getString("jump_anim");
        _climbingCharacterComponentId = properties.getString("climb_anim");
        _movementSpeed = properties.getFloat("speed");
        _jumpHeight = properties.getFloat("jump_height");
        _normalCharacterComponentId = properties.getString("normal_physics");
    }

    PlayerComponent::State::Enum PlayerComponent::getState() const
    {
        return _state;
    }

    gameplay::Vector2 PlayerComponent::getPosition() const
    {
        return gameplay::Vector2(_characterNode->getTranslation().x, _characterNode->getTranslation().y);
    }

    void PlayerComponent::forEachAnimation(std::function <bool(State::Enum, SpriteAnimationComponent *)> func)
    {
        for (auto & pair : _animations)
        {
            if (func(pair.first, pair.second))
            {
                break;
            }
        }
    }

    void PlayerComponent::update(float elapsedTime)
    {
        if(_characterNode->getCollisionObject()->asCharacter()->isPhysicsEnabled())
        {
            gameplay::Vector3 velocity = _characterNode->getCollisionObject()->asCharacter()->getCurrentVelocity();

            if(_movementDirection == MovementDirection::None || (_movementDirection & MovementDirection::Vertical) != MovementDirection::None)
            {
                if (velocity.y == 0.0f && velocity.x != 0.0f)
                {
                    velocity = gameplay::Vector3::zero();
                }
            }

            if (velocity.isZero())
            {
                if(_movementDirection == MovementDirection::None || _movementDirection == MovementDirection::Up)
                {
                    _state = State::Idle;
                }
            }
            else
            {
                if(velocity.y == 0.0f && velocity.x != 0.0f)
                {
                    _state = State::Walking;
                }

                float const minFallVelocity = -2.0f;

                if(velocity.y < minFallVelocity && (_state == State::Idle || _state == State::Walking))
                {
                    _state = State::Cowering;
                }
            }

            if(velocity != _characterNode->getCollisionObject()->asCharacter()->getCurrentVelocity())
            {
                velocity.z = 0.0f;
                _characterNode->getCollisionObject()->asCharacter()->setVelocity(velocity);
            }
        }
        else
        {
            if(_state == State::Climbing)
            {
                float const elapsedTimeMs = elapsedTime / 1000.0f;
                float const verticalMovementSpeed = _movementSpeed / 2.0f;
                _characterNode->translateY((_movementScale * verticalMovementSpeed) * elapsedTimeMs);
            }
        }

        if(_state != _previousState)
        {
            _animations[_previousState]->stop();
            getCurrentAnimation()->play();
        }

        if(_state == State::Walking || _state == State::Climbing)
        {
            getCurrentAnimation()->setSpeed(_movementScale);
        }

        _characterNode->setTranslationZ(0);

        _previousState = _state;

        _camera->setTargetPosition(getPosition(), elapsedTime);
    }

    SpriteAnimationComponent * PlayerComponent::getCurrentAnimation()
    {
        return _animations[_state];
    }

    gameplay::Node * PlayerComponent::getCharacterNode() const
    {
        return _characterNode;
    }

    void PlayerComponent::setMovementEnabled(MovementDirection::Enum direction, bool enabled, float scale /* = 1.0f */)
    {
        if(enabled)
        {
            float const minDownScale = 0.75f;

            if (direction != MovementDirection::Down || scale > minDownScale)
            {
                _movementDirection = direction;

                if(direction == MovementDirection::Up && _climbingEnabled)
                {
                    _characterNode->setTranslationX(_climbingSnapPositionX);

                    float const velocityY =  _characterNode->getCollisionObject()->asCharacter()->getCurrentVelocity().y;
                    float const maxVelocityYDelta = 0.1f;

                    if(velocityY >= 0.0f && velocityY <= (velocityY + maxVelocityYDelta))
                    {
                        _state = State::Climbing;
                        _characterNode->getCollisionObject()->asCharacter()->setVelocity(gameplay::Vector3::zero());
                        _characterNode->getCollisionObject()->asCharacter()->setPhysicsEnabled(false);
                    }
                }
                else
                {
                    if(_state == State::Climbing)
                    {
                        _characterNode->getCollisionObject()->asCharacter()->setPhysicsEnabled(true);
                        _state = State::Idle;
                    }

                    if((direction & MovementDirection::Horizontal) == direction)
                    {
                        _isLeftFacing = direction == MovementDirection::Left;
                        float horizontalSpeed = _isLeftFacing ? -_movementSpeed : _movementSpeed;
                        horizontalSpeed *= scale;

                        _characterNode->getCollisionObject()->asCharacter()->setVelocity(horizontalSpeed, 0.0f, 0.0f);
                    }
                }

                _movementScale = scale;
            }
        }
        else
        {
            if(_movementDirection == direction)
            {
                if(direction == MovementDirection::Up)
                {
                    _movementScale = 0.0f;
                }
                else
                {
                    _movementDirection = MovementDirection::None;
                }
            }
        }
    }

    void PlayerComponent::setClimbingEnabled(bool enabled)
    {
        if(_climbingEnabled && !enabled && _state == State::Climbing)
        {
            _movementScale = 0.0f;
        }

        _climbingEnabled = enabled;
    }

    void PlayerComponent::setClimpingSnapPositionX(float posX)
    {
        _climbingSnapPositionX = posX;
    }

    void PlayerComponent::jump(float scale)
    {
        if(_characterNode->getCollisionObject()->asCharacter()->getCurrentVelocity().y == 0.0f && _state != State::Climbing)
        {
            _state = State::Jumping;
            _characterNode->getCollisionObject()->asCharacter()->setPhysicsEnabled(true);
            _characterNode->getCollisionObject()->asCharacter()->jump(_jumpHeight * scale);
            getParent()->broadcastMessage(_jumpMessage);
        }
    }

    bool PlayerComponent::IsLeftFacing() const
    {
        return _isLeftFacing;
    }
}
