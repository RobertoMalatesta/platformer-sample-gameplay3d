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
        , _swimmingEnabled(false)
        , _climbingSnapPositionX(0.0f)
        , _swimSpeedScale(1.0f)
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
        _animations[State::Swimming] = getParent()->findComponent<SpriteAnimationComponent>(_swimmingCharacterComponentId);

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
        _swimmingCharacterComponentId = properties.getString("swim_anim");
        _movementSpeed = properties.getFloat("speed");
        _swimSpeedScale = properties.getFloat("swim_speed_scale");
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
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_characterNode->getCollisionObject());

        if(character->isPhysicsEnabled())
        {
            gameplay::Vector3 velocity = character->getCurrentVelocity();

            if(_movementDirection == MovementDirection::None || (_movementDirection & MovementDirection::Vertical) != MovementDirection::None)
            {
                if (velocity.y == 0.0f && velocity.x != 0.0f)
                {
                    velocity = gameplay::Vector3::zero();
                }
            }

            if (_state != State::Swimming)
            {
                if (velocity.isZero())
                {
                    if(_movementDirection == MovementDirection::None || _movementDirection == MovementDirection::Up)
                    {
                        _state = _swimmingEnabled ? State::Swimming : State::Idle;
                    }
                }
                else
                {
                    if (velocity.y == 0.0f && velocity.x != 0.0f)
                    {
                        if (_swimmingEnabled && _state == State::Walking)
                        {
                            character->setVelocity(character->getCurrentVelocity().x * _swimSpeedScale, 0, 0);
                            _state = State::Swimming;
                        }
                        else
                        {
                            _state = State::Walking;
                        }
                    }

                    float const minFallVelocity = -2.0f;

                    if (velocity.y < minFallVelocity && (_state == State::Idle || _state == State::Walking))
                    {
                        _state = State::Cowering;
                    }
                }
            }

            if(velocity != character->getCurrentVelocity())
            {
                velocity.z = 0.0f;
                character->setVelocity(velocity);
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

                float const minDistToLadderCentre = std::pow(_characterNode->getScaleX() / 2.0f, 2);
                gameplay::Vector3 const ladderPos = gameplay::Vector3(_climbingSnapPositionX, _characterNode->getTranslationY(), 0.0f);
                gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_characterNode->getCollisionObject());

                if(direction == MovementDirection::Up && _climbingEnabled && _characterNode->getTranslation().distanceSquared(ladderPos) <= minDistToLadderCentre)
                {
                    float const velocityY =  character->getCurrentVelocity().y;
                    float const maxVelocityYDelta = 0.1f;

                    if(velocityY >= 0.0f && velocityY <= (velocityY + maxVelocityYDelta))
                    {
                        _state = State::Climbing;
                        character->setVelocity(gameplay::Vector3::zero());
                        character->setPhysicsEnabled(false);
                    }
                }
                else
                {
                    if(_state == State::Climbing)
                    {
                        character->setPhysicsEnabled(true);
                        _state = State::Idle;
                    }
                    else if (_swimmingEnabled)
                    {
                        _state = State::Swimming;
                    }

                    if((direction & MovementDirection::Horizontal) == direction)
                    {
                        _isLeftFacing = direction == MovementDirection::Left;
                        float horizontalSpeed = _isLeftFacing ? -_movementSpeed : _movementSpeed;
                        horizontalSpeed *= scale;

                        if (_state == State::Swimming)
                        {
                            horizontalSpeed *= _swimSpeedScale;
                        }

                        character->setVelocity(horizontalSpeed, 0.0f, 0.0f);
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

    void PlayerComponent::setSwimmingEnabled(bool enabled)
    {
        if (!enabled && _swimmingEnabled)
        {
            gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_characterNode->getCollisionObject());

            if (character->getCurrentVelocity().x != 0)
            {
                character->setVelocity(character->getCurrentVelocity().x / _swimSpeedScale, 0 , 0);
            }
        }

        _swimmingEnabled = enabled;
    }

    void PlayerComponent::setClimpingSnapPositionX(float posX)
    {
        _climbingSnapPositionX = posX;
    }

    void PlayerComponent::jump(JumpSource::Enum source, float scale)
    {
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_characterNode->getCollisionObject());
        gameplay::Vector3 const characterOriginalVelocity = character->getCurrentVelocity();
        gameplay::Vector3 preJumpVelocity;
        bool jumpAllowed = _state != State::Climbing;
        float jumpHeight = (_jumpHeight * scale);
        bool resetVelocityState = true;

        switch(source)
        {
            case JumpSource::Input:
            {
                preJumpVelocity.x = characterOriginalVelocity.x;
                jumpAllowed &= character->getCurrentVelocity().y == 0.0f;
#ifndef _FINAL
                if(gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_unlimited_jump"))
                {
                    if(characterOriginalVelocity.y < 0)
                    {
                        jumpHeight += -characterOriginalVelocity.y / 2.0f;
                    }

                    resetVelocityState = false;
                    jumpAllowed = true;
                }
#endif
                break;
            }
            case JumpSource::EnemyCollision:
            {
                preJumpVelocity.x = characterOriginalVelocity.x;
                jumpAllowed &= true;
                break;
            }
            default:
                PLATFORMER_ASSERTFAIL("Unhandled JumpSource %d", source);
                break;
        }

        if(jumpAllowed)
        {
            _state = State::Jumping;
            character->setPhysicsEnabled(true);

            if(resetVelocityState)
            {
                character->resetVelocityState();
            }

            character->setVelocity(preJumpVelocity);
            character->jump(jumpHeight, !resetVelocityState);
            getParent()->broadcastMessage(_jumpMessage);
        }
    }

    bool PlayerComponent::isLeftFacing() const
    {
        return _isLeftFacing;
    }
}
