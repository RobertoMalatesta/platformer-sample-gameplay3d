#include "CollisionHandlerComponent.h"

#include "Common.h"
#include "EnemyComponent.h"
#include "GameObject.h"
#include "LevelLoaderComponent.h"
#include "PlayerComponent.h"
#include "Messages.h"
#include "PhysicsCharacter.h"
#include "LevelCollision.h"

namespace game
{
    CollisionHandlerComponent::CollisionHandlerComponent()
        : Component(true)
        , _player(nullptr)
        , _playerClimbingTerrainRefCount(0)
        , _playerSwimmingRefCount(0)
        , _waitForPhysicsCleanup(false)
        , _framesSinceLevelReloaded(0)
        , _forceHandOfGodMessage(nullptr)
        , _enemyKilledMessage(nullptr)
        , _playerTriggerNode(nullptr)
        , _playerPhysicsNode(nullptr)
        , _enemyCollisionListener(nullptr)
        , _playerCollisionListener(nullptr)
    {
    }

    CollisionHandlerComponent::~CollisionHandlerComponent()
    {
    }

    bool CollisionHandlerComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
        switch (messageType)
        {
        case(Messages::Type::LevelLoaded):
            onLevelUnloaded();
            onLevelLoaded();
            break;
        case(Messages::Type::PreLevelUnloaded):
        case(Messages::Type::LevelUnloaded):
            onLevelUnloaded();
            break;
        }

        return true;
    }

    void addOrRemoveCollisionListener(collision::Type::Enum collisionType,
                                      gameplay::PhysicsRigidBody::CollisionListener * listener,
                                      LevelLoaderComponent * level,
                                      gameplay::PhysicsCollisionObject * collisionObject,
                                      bool add)
    {
        level->forEachCachedNode(collisionType, [&add, listener, &collisionObject](gameplay::Node * node)
        {
            if(add)
            {
                collisionObject->addCollisionListener(listener, node->getCollisionObject());
            }
            else
            {
                collisionObject->removeCollisionListener(listener, node->getCollisionObject());
            }

        });
    }

    void CollisionHandlerComponent::onLevelLoaded()
    {
        _playerClimbingTerrainRefCount = 0;
        _playerSwimmingRefCount = 0;
        _waitForPhysicsCleanup = true;
        std::vector<EnemyComponent *> enemyComponents;
        getParent()->getComponentsInChildren(enemyComponents);

        _player = getParent()->getComponentInChildren<PlayerComponent>();
        _player->addRef();
        _playerPhysicsNode = _player->getPhysicsNode();
        _playerPhysicsNode->addRef();
        _playerTriggerNode = _player->getTriggerNode();
        _playerTriggerNode->addRef();

        for(EnemyComponent * enemy  : enemyComponents)
        {
            enemy->addRef();
            gameplay::Node * enemyNode = enemy->getTriggerNode();
            enemyNode->addRef();
            gameplay::PhysicsCollisionObject * enemyPhysics = enemyNode->getCollisionObject();
            _enemies[enemyPhysics] = enemy;
        }

        LevelLoaderComponent * level = getParent()->getComponent<LevelLoaderComponent>();
        addOrRemoveCollisionListener(collision::Type::LADDER, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), true);
        addOrRemoveCollisionListener(collision::Type::RESET, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), true);
        addOrRemoveCollisionListener(collision::Type::COLLECTABLE, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), true);
        addOrRemoveCollisionListener(collision::Type::WATER, _playerCollisionListener, level, _playerPhysicsNode->getCollisionObject(), true);
        addOrRemoveCollisionListener(collision::Type::KINEMATIC, _playerCollisionListener, level, _playerPhysicsNode->getCollisionObject(), true);

        for (auto & enemyPair : _enemies)
        {
            EnemyComponent * enemyComponent = enemyPair.second;
            enemyComponent->getTriggerNode()->getCollisionObject()->addCollisionListener(_enemyCollisionListener, _playerTriggerNode->getCollisionObject());
        }
    }

    void CollisionHandlerComponent::onLevelUnloaded()
    {
        LevelLoaderComponent * level = getParent()->getComponent<LevelLoaderComponent>();

        for(auto & enemyPair  : _enemies)
        {
            EnemyComponent * enemyComponent = enemyPair.second;
            gameplay::PhysicsCollisionObject * enemyPhysics = enemyPair.first;
            gameplay::Node * enemyNode = enemyPhysics->getNode();

            _playerTriggerNode->getCollisionObject()->removeCollisionListener(_enemyCollisionListener, _playerTriggerNode->getCollisionObject());

            SAFE_RELEASE(enemyNode);
            SAFE_RELEASE(enemyComponent);
        }

        _enemies.clear();

        if(_playerPhysicsNode)
        {
            addOrRemoveCollisionListener(collision::Type::LADDER, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), false);
            addOrRemoveCollisionListener(collision::Type::RESET, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), false);
            addOrRemoveCollisionListener(collision::Type::COLLECTABLE, _playerCollisionListener, level, _playerTriggerNode->getCollisionObject(), false);
            addOrRemoveCollisionListener(collision::Type::WATER, _playerCollisionListener, level, _playerPhysicsNode->getCollisionObject(), false);
            addOrRemoveCollisionListener(collision::Type::KINEMATIC, _playerCollisionListener, level, _playerPhysicsNode->getCollisionObject(), false);
        }

        SAFE_RELEASE(_playerPhysicsNode);
        SAFE_RELEASE(_playerTriggerNode);
        SAFE_RELEASE(_player);
    }

    void CollisionHandlerComponent::update(float elapsedTime)
    {
        if(_waitForPhysicsCleanup)
        {
            ++_framesSinceLevelReloaded;

            if(_framesSinceLevelReloaded > 1)
            {
                _framesSinceLevelReloaded = 0;
                _waitForPhysicsCleanup = false;
            }
        }
    }

    void CollisionHandlerComponent::initialize()
    {
        _forceHandOfGodMessage = PlayerForceHandOfGodResetMessage::create();
        _enemyKilledMessage = EnemyKilledMessage::create();
        _playerCollisionListener = new PlayerCollisionListener();
        _playerCollisionListener->_collisionHandler = this;
        _enemyCollisionListener = new EnemyCollisionListener();
        _enemyCollisionListener->_collisionHandler = this;
    }

    void CollisionHandlerComponent::finalize()
    {
        onLevelUnloaded();
        GAMEOBJECTS_DELETE_MESSAGE(_forceHandOfGodMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_enemyKilledMessage);
        SAFE_RELEASE(_playerCollisionListener);
        SAFE_RELEASE(_enemyCollisionListener);
    }

    void CollisionHandlerComponent::EnemyCollisionListener::collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                        gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                        gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB)
    {
        if(!_collisionHandler->_waitForPhysicsCleanup)
        {
            _collisionHandler->onEnemyCollision(type, collisionPair, contactPointA, contactPointB);
        }
    }

    void CollisionHandlerComponent::PlayerCollisionListener::collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                        gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                        gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB)
    {
        if(!_collisionHandler->_waitForPhysicsCleanup)
        {
            _collisionHandler->onPlayerCollision(type, collisionPair, contactPointA, contactPointB);
        }
    }

    void CollisionHandlerComponent::onEnemyCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                                                     gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                                                     gameplay::Vector3 const &, gameplay::Vector3 const &)
    {
        if(type == gameplay::PhysicsCollisionObject::CollisionListener::EventType::COLLIDING)
        {
            switch(collisionPair.objectB->getType())
            {
            case gameplay::PhysicsCollisionObject::Type::GHOST_OBJECT:
                {
                    EnemyComponent * enemy = gameobjects::GameObject::getGameObject(collisionPair.objectA->getNode()->getParent())->getComponent<EnemyComponent>();
                    if(enemy->getState() != EnemyComponent::State::Dead)
                    {
                        float const height = std::min(collisionPair.objectA->getNode()->getScaleY(), collisionPair.objectB->getNode()->getScaleY()) * 0.5f;
                        gameplay::Rectangle const playerBottom(_player->getRenderPosition().x - ( collisionPair.objectB->getNode()->getScaleX() / 2),
                                                         _player->getRenderPosition().y - (collisionPair.objectB->getNode()->getScaleY() / 2),
                                                         collisionPair.objectB->getNode()->getScaleX(), height);

                        gameplay::Rectangle const enemyTop(enemy->getPosition().x -  (( collisionPair.objectA->getNode()->getScaleX()) / 2),
                                                         enemy->getPosition().y + (collisionPair.objectA->getNode()->getScaleY() / 2) - (collisionPair.objectA->getNode()->getScaleY() * 0.2f),
                                                         collisionPair.objectA->getNode()->getScaleX(), height);

                        if(playerBottom.intersects(enemyTop))
                        {
                            gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(_player->getPhysicsNode()->getCollisionObject());

                            if(character->getCurrentVelocity().y <= 0)
                            {
                                enemy->kill();
                                getRootParent()->broadcastMessage(_enemyKilledMessage);
                                _player->jump(PlayerComponent::JumpSource::EnemyCollision);
                            }
                        }
                        else
                        {
                            getRootParent()->broadcastMessage(_forceHandOfGodMessage);
                        }
                    }
                }
                break;
            case gameplay::PhysicsCollisionObject::Type::RIGID_BODY:
            case gameplay::PhysicsCollisionObject::Type::CHARACTER:
            case gameplay::PhysicsCollisionObject::Type::VEHICLE:
            case gameplay::PhysicsCollisionObject::Type::VEHICLE_WHEEL:
            case gameplay::PhysicsCollisionObject::Type::NONE:
                break;
            default:
                GAME_ASSERTFAIL("Unhandled PhysicsCollisionObject type %d", collisionPair.objectB->getType());
            }
        }
    }

    void CollisionHandlerComponent::onPlayerCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                        gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                        gameplay::Vector3 const &, gameplay::Vector3 const &)
    {
        if(collision::NodeData * NodeCollisionInfo = collision::NodeData::get(collisionPair.objectB->getNode()))
        {
            bool const isColliding = type == gameplay::PhysicsCollisionObject::CollisionListener::EventType::COLLIDING;

            switch (NodeCollisionInfo->_type)
            {
                case collision::Type::LADDER:
                {
                    if (isColliding)
                    {
                        _player->setLadderPosition(collisionPair.objectB->getNode()->getTranslation());
                        ++_playerClimbingTerrainRefCount;
                    }
                    else
                    {
                        --_playerClimbingTerrainRefCount;
                    }

                    GAME_ASSERT(_playerClimbingTerrainRefCount == MATH_CLAMP(_playerClimbingTerrainRefCount, 0, std::numeric_limits<int>::max()),
                        "_playerClimbingTerrainRefCount invalid %d", _playerClimbingTerrainRefCount);
                    _player->setClimbingEnabled(_playerClimbingTerrainRefCount > 0);
                    break;
                }
                case collision::Type::RESET:
                {
                    if (isColliding)
                    {
                        getRootParent()->broadcastMessage(_forceHandOfGodMessage);
                    }
                    break;
                }
                case collision::Type::COLLECTABLE:
                {
                    LevelLoaderComponent * level = getParent()->getComponent<LevelLoaderComponent>();
                    level->consumeCollectable(collisionPair.objectB->getNode());
                    break;
                }
                case collision::Type::WATER:
                {
                    isColliding ? ++_playerSwimmingRefCount : --_playerSwimmingRefCount;
                    _player->setSwimmingEnabled(_playerSwimmingRefCount > 0);
                    GAME_ASSERT(_playerSwimmingRefCount == MATH_CLAMP(_playerSwimmingRefCount, 0, std::numeric_limits<int>::max()),
                        "_playerSwimmingRefCount invalid %d", _playerSwimmingRefCount);
                    break;
                }
                case collision::Type::KINEMATIC:
                {
                    _player->setIntersectingKinematic(isColliding ? collisionPair.objectB->getNode() : nullptr);
                    break;
                }
                default:
                    GAME_ASSERTFAIL("Unhandled terrain collision type %d", NodeCollisionInfo->_type);
            }
        }
    }
}
