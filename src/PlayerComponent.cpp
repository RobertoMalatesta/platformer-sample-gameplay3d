#include "PlayerComponent.h"

#include "Common.h"
#include "CollisionObjectComponent.h"
#include "Game.h"
#include "GameObject.h"
#include "Messages.h"
#include "PlayerInputComponent.h"
#include "PlayerHandOfGodComponent.h"
#include "PhysicsCharacter.h"
#include "Properties.h"
#include "PropertiesRef.h"
#include "ResourceManager.h"
#include "SpriteAnimationComponent.h"

namespace game
{
    PlayerComponent::PlayerComponent()
        : _isLeftFacing(false)
        , _horizontalMovementDirection(MovementDirection::None)
        , _previousState(State::Idle)
        , _movementSpeed(5.0f)
        , _jumpHeight(1.0f)
        , _physicsNode(nullptr)
        , _horizontalMovementScale(0.0f)
        , _verticalMovementScale(0.0f)
        , _jumpMessage(nullptr)
        , _climbingEnabled(false)
        , _swimmingEnabled(false)
        , _swimSpeedScale(1.0f)
        , _playerInputComponent(nullptr)
        , _playerHandOfGodComponent(nullptr)
        , _kinematicNode(nullptr)
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

        _physicsNode = getParent()->findComponent<CollisionObjectComponent>(_physicsComponentId)->getNode();
        _physicsNode->addRef();
        _physicsNode->getParent()->removeChild(_physicsNode);
        getParent()->getNode()->addChild(_physicsNode);

        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());
        character->setClampVerticalVelocityToGravity(true);
        character->setAllowHorizontalCorrectionOnStepDown(false);

        _triggerNode = getParent()->findComponent<CollisionObjectComponent>(_triggerComponentId)->getNode();
        _triggerNode->addRef();

        _jumpMessage = PlayerJumpMessage::create();
        _state = State::Idle;

        _playerHandOfGodComponent = getParent()->getComponent<PlayerHandOfGodComponent>();
        GAME_SAFE_ADD(_playerHandOfGodComponent);
        _playerInputComponent = getParent()->getComponent<PlayerInputComponent>();
        GAME_SAFE_ADD(_playerInputComponent);
    }

    void PlayerComponent::finalize()
    {
        getParent()->getNode()->removeAllChildren();

        if(_kinematicNode)
        {
            _kinematicNode->getParent()->removeAllChildren();
            SAFE_RELEASE(_kinematicNode);
        }

        SAFE_RELEASE(_triggerNode);
        SAFE_RELEASE(_physicsNode);
        SAFE_RELEASE(_playerHandOfGodComponent);
        SAFE_RELEASE(_playerInputComponent);
        GAMEOBJECTS_DELETE_MESSAGE(_jumpMessage);
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
        _physicsComponentId = properties.getString("physics");
        _triggerComponentId = properties.getString("trigger");
    }

    PlayerComponent::State::Enum PlayerComponent::getState() const
    {
        return _state;
    }

    gameplay::Vector2 PlayerComponent::getPosition() const
    {
        return gameplay::Vector2(_physicsNode->getTranslation().x + _physicsNode->getParent()->getTranslation().x,
                                 _physicsNode->getTranslation().y + _physicsNode->getParent()->getTranslation().y);
    }

    gameplay::Vector2 PlayerComponent::getRenderPosition() const
    {
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());
        return gameplay::Vector2(character->getInterpolatedPosition().x, character->getInterpolatedPosition().y);
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
        if(_playerInputComponent)
        {
            _playerInputComponent->update();
        }

        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());
        gameplay::Vector3 velocity = character->getCurrentVelocity();
        float const minVerticalScaleToInitiateClimb = 0.35f;
        float const minDistToLadderCentre = _physicsNode->getScaleX() * 0.15f;
        gameplay::Vector2 const ladderVeritcallyAlignedPosition = gameplay::Vector2(_ladderPosition.x, getRenderPosition().y);
        bool const isClimbRequested = fabs(_verticalMovementScale) > minVerticalScaleToInitiateClimb;
        bool const isPlayerWithinLadderClimbingDistance = getRenderPosition().distance(ladderVeritcallyAlignedPosition) <= minDistToLadderCentre;

        // Initiate climbing if possible
        if(_climbingEnabled && isClimbRequested && isPlayerWithinLadderClimbingDistance)
        {
            _state = State::Climbing;

            // Physics will be disabled so that we can translate the player vertically along the ladder
            _physicsNode->setTranslationX(_ladderPosition.x);
            character->resetVelocityState();
            character->setPhysicsEnabled(false);
        }

        if(character->isPhysicsEnabled())
        {
            // Zero velocity once the player has stopped falling and there isn't a desired movement direction
            if(_horizontalMovementDirection == MovementDirection::None)
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
                    // Player should swim on the spot when idle while in water
                    if(_horizontalMovementDirection == MovementDirection::None)
                    {
                        _state = _swimmingEnabled ? State::Swimming : State::Idle;
                    }
                }
                else
                {
                    // Handle when the player moves horizontally
                    if (velocity.y == 0.0f && velocity.x != 0.0f)
                    {
                        float moveScale = _horizontalMovementScale;

                        if (_swimmingEnabled && _state == State::Walking)
                        {
                            // Player has moved into a water volume, transition from walking to swimming and move at swim speed
                            _state = State::Swimming;
                            moveScale *= _swimSpeedScale;
                        }
                        else
                        {
                            // Player has transitioned to walking from swimming
                            _state = State::Walking;
                        }

                        velocity.x = (_isLeftFacing ? -_movementSpeed : _movementSpeed) * moveScale;
                    }

                    // Make the player cower when they are pushed/walk off a ledge
                    float const minFallVelocity = gameplay::Game::getInstance()->getPhysicsController()->getGravity().y * 0.25f;

                    if (velocity.y < minFallVelocity && (_state == State::Idle || _state == State::Walking))
                    {
                        _state = State::Cowering;
                    }
                }
            }

            // Apply new velocity if different
            if(velocity != character->getCurrentVelocity())
            {
                velocity.z = 0.0f;
                character->setVelocity(velocity);
            }
        }
        else
        {
            // Move the player along the ladder using the input vertical movement scale
            float const elapsedTimeMs = elapsedTime / 1000.0f;
            float const verticalMovementSpeed = _movementSpeed / 2.0f;
            float const previousDistToLadder = _ladderPosition.distanceSquared(_physicsNode->getTranslation());
            gameplay::Vector3 const previousPosition = _physicsNode->getTranslation();
            _physicsNode->translateY((_verticalMovementScale * verticalMovementSpeed) * elapsedTimeMs);

            // If the player has moved away from the ladder but they are no longer intersecting it then restore their last position
            // and zero their movement, this will prevent them from climing beyond the top/bottom
            if(!_climbingEnabled && previousDistToLadder < _physicsNode->getTranslation().distanceSquared(_ladderPosition))
            {
                _physicsNode->setTranslation(previousPosition);
                _verticalMovementScale = 0.0f;
            }
        }

        // Play animation if different
        if(_state != _previousState)
        {
            _animations[_previousState]->stop();
            getCurrentAnimation()->play();
        }

        // Scale the animation speed using the movement scale
        if(_state == State::Walking || _state == State::Climbing)
        {
            getCurrentAnimation()->setSpeed(fabs(_state == State::Climbing ? _verticalMovementScale : _horizontalMovementScale));
        }

        _physicsNode->setTranslationZ(0);

        _previousState = _state;

        getCurrentAnimation()->update(elapsedTime);

        if(_playerHandOfGodComponent)
        {
            _playerHandOfGodComponent->update(elapsedTime);
        }

        character->update(elapsedTime);
        _triggerNode->setTranslation(getRenderPosition().x, getRenderPosition().y, 0);
    }

    SpriteAnimationComponent * PlayerComponent::getCurrentAnimation()
    {
        return _animations[_state];
    }

    gameplay::Node * PlayerComponent::getPhysicsNode() const
    {
        return _physicsNode;
    }

    gameplay::Node * PlayerComponent::getTriggerNode() const
    {
        return _triggerNode;
    }

    void PlayerComponent::setMovementEnabled(MovementDirection::Enum direction, bool enabled, float scale /* = 1.0f */)
    {
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());

        if(enabled)
        {
            if(direction & MovementDirection::Horizontal)
            {
                // Apply state based on horizontal movement immediatley

                _horizontalMovementDirection = direction;

                float const minHorizontalScaleToCancelClimb = 0.75f;

                if(_state == State::Climbing && scale > minHorizontalScaleToCancelClimb)
                {
                    // Player was climbing a ladder but they moved enough horizontally that climbing should be cancelled
                    _verticalMovementScale = 0.0f;
                    character->setPhysicsEnabled(true);
                    _state = State::Cowering;
                }
                else if (_swimmingEnabled && character->getCurrentVelocity().y == 0)
                {
                    // Player is colliding with a swimming surface
                    _state = State::Swimming;
                }

                // Apply the horizontal velocity and face the player in the direction of the input provided they aren't climbing
                if(_state != State::Climbing)
                {
                    _isLeftFacing = direction == MovementDirection::Left;
                    float horizontalSpeed = _isLeftFacing ? -_movementSpeed : _movementSpeed;
                    horizontalSpeed *= _state == State::Swimming ? scale * _swimSpeedScale : scale;
                    character->setVelocity(horizontalSpeed, 0.0f, 0.0f);
                    _horizontalMovementScale = scale;
                }
            }
            else
            {
                // Cache the desired vertical movement, state will only be applied in update since we want the player
                // to be able to apply both horizontal and vertical movement at the same time
                _verticalMovementScale = scale * (direction == MovementDirection::Up ? 1.0f : -1.0f);
            }
        }
        else
        {
            // Zero mutually exclusive pairs left/right and up/down if input to disable matches current movement scale

            if(direction & MovementDirection::Vertical)
            {
                if(direction & MovementDirection::Up && _verticalMovementScale > 0 || direction & MovementDirection::Down && _verticalMovementScale < 0)
                {
                    _verticalMovementScale = 0.0f;
                }
            }
            else if(_horizontalMovementDirection == direction)
            {
                _horizontalMovementDirection = MovementDirection::None;
                _horizontalMovementScale = 0.0f;
            }
        }
    }

    void PlayerComponent::attachToKinematic()
    {
        _physicsNode->setTranslation(_physicsNode->getTranslation() - _kinematicNode->getParent()->getTranslation());
        _kinematicNode->getParent()->addChild(_physicsNode);
    }

    void PlayerComponent::detachFromKinematic()
    {
        _physicsNode->setTranslation(_physicsNode->getTranslation() + _kinematicNode->getParent()->getTranslation());
        _kinematicNode->getParent()->removeChild(_physicsNode);
        getParent()->getNode()->addChild(_physicsNode);
        SAFE_RELEASE(_kinematicNode);
    }

    void PlayerComponent::setIntersectingKinematic(gameplay::Node * node)
    {
        if(node)
        {
            if(!_kinematicNode || _kinematicNode != node)
            {
                if(_kinematicNode)
                {
                    detachFromKinematic();
                }

                node->addRef();
                _kinematicNode = node;

                attachToKinematic();
            }
        }
        else if(_kinematicNode)
        {
            detachFromKinematic();
        }
    }

    void PlayerComponent::setClimbingEnabled(bool enabled)
    {
        if(_climbingEnabled && !enabled && _state == State::Climbing)
        {
            _verticalMovementScale = 0.0f;
        }

        _climbingEnabled = enabled;
    }

    void PlayerComponent::setSwimmingEnabled(bool enabled)
    {
        if(!enabled && _swimmingEnabled && _state == State::Swimming)
        {
            _state = State::Idle;
        }

        _swimmingEnabled = enabled;
    }

    void PlayerComponent::setLadderPosition(gameplay::Vector3 const & pos)
    {
        _ladderPosition = pos;
    }

    void PlayerComponent::jump(JumpSource::Enum source, float scale)
    {
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());
        gameplay::Vector3 const characterOriginalVelocity = character->getCurrentVelocity();
        gameplay::Vector3 preJumpVelocity;
        bool jumpAllowed = characterOriginalVelocity.y == 0;
        float jumpHeight = (_jumpHeight * scale);
        bool resetVelocityState = true;

        switch(source)
        {
            case JumpSource::Input:
            {
                preJumpVelocity.x = _state != State::Swimming || characterOriginalVelocity.x == 0 ? characterOriginalVelocity.x :
                    (_movementSpeed * _swimSpeedScale) * (!_isLeftFacing ? 1.0f : -1.0f);
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
                float const verticalModifier = 1.5f;
                jumpHeight *= verticalModifier;
                jumpAllowed = true;
                break;
            }
            default:
                GAME_ASSERTFAIL("Unhandled JumpSource %d", source);
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
            if(source != JumpSource::EnemyCollision)
            {
                getRootParent()->broadcastMessage(_jumpMessage);
            }
            _verticalMovementScale = 0.0f;
        }
    }

    bool PlayerComponent::isLeftFacing() const
    {
        return _isLeftFacing;
    }

    void PlayerComponent::reset(gameplay::Vector2 const position)
    {
        _state = State::Idle;
        _isLeftFacing = false;
        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_physicsNode->getCollisionObject());
        _physicsNode->setTranslation(gameplay::Vector3(position.x, position.y, 0));
        character->resetVelocityState();
        character->setPhysicsEnabled(true);
    }
}
