#include "CollisionHandlerComponent.h"

#include "EnemyComponent.h"
#include "GameObject.h"
#include "LevelComponent.h"
#include "PlayerComponent.h"
#include "Messages.h"
#include "MessagesLevel.h"
#include "MessagesPlayer.h"
#include "TerrainInfo.h"

namespace platformer
{
    CollisionHandlerComponent::CollisionHandlerComponent()
        : Component(true)
        , _player(nullptr)
        , _playerClimbingTerrainRefCount(0)
        , _waitForPhysicsCleanup(false)
        , _framesSinceLevelReloaded(0)
        , _forceHandOfGodMessage(nullptr)
    {
    }

    CollisionHandlerComponent::~CollisionHandlerComponent()
    {
    }

    void CollisionHandlerComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        switch (message->getId())
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
    }

    void CollisionHandlerComponent::onLevelLoaded()
    {
        _waitForPhysicsCleanup = true;
        std::vector<EnemyComponent *> enemyComponents;
        getParent()->getComponentsInChildren(enemyComponents);

        _player = getParent()->getComponentInChildren<PlayerComponent>();
        _player->addRef();
        gameplay::Node * playerCharacterNode = _player->getCharacterNode();

        playerCharacterNode->addRef();
        _playerCharacterNodes.insert(playerCharacterNode);
        LevelComponent * level = getParent()->getComponent<LevelComponent>();

        for(EnemyComponent * enemy  : enemyComponents)
        {
            enemy->addRef();
            gameplay::Node * enemyNode = enemy->getTriggerNode();
            enemyNode->addRef();
            gameplay::PhysicsCollisionObject * enemyPhysics = enemyNode->getCollisionObject();
            _enemies[enemyPhysics] = enemy;
        }

        addPlayerCollisionListeners(playerCharacterNode->getCollisionObject());
    }

    void addOrRemoveCollisionListener(CollisionType::Enum collisionType,
                                      gameplay::PhysicsRigidBody::CollisionListener * listener,
                                      LevelComponent * level,
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

    void CollisionHandlerComponent::onLevelUnloaded()
    {
        LevelComponent * level = getParent()->getComponent<LevelComponent>();

        for(auto & enemyPair  : _enemies)
        {
            EnemyComponent * enemyComponent = enemyPair.second;
            gameplay::PhysicsCollisionObject * enemyPhysics = enemyPair.first;
            gameplay::Node * enemyNode = enemyPhysics->getNode();

            for (gameplay::Node * node : _playerCharacterNodes)
            {
                node->getCollisionObject()->removeCollisionListener(this, node->getCollisionObject());
            }

            SAFE_RELEASE(enemyNode);
            SAFE_RELEASE(enemyComponent);
        }

        _enemies.clear();

        for(gameplay::Node * node : _playerCharacterNodes)
        {
            gameplay::PhysicsCollisionObject * playerCollisionObject = node->getCollisionObject();
            addOrRemoveCollisionListener(CollisionType::LADDER, this, level, playerCollisionObject, false);
            addOrRemoveCollisionListener(CollisionType::RESET, this, level, playerCollisionObject, false);
            addOrRemoveCollisionListener(CollisionType::COLLECTABLE, this, level, playerCollisionObject, false);

            SAFE_RELEASE(node);
        }

        _playerCharacterNodes.clear();

        SAFE_RELEASE(_player);
    }

    void CollisionHandlerComponent::addPlayerCollisionListeners(gameplay::PhysicsCollisionObject * playerCollisionObject)
    {
        LevelComponent * level = getParent()->getComponent<LevelComponent>();
        addOrRemoveCollisionListener(CollisionType::LADDER, this, level, playerCollisionObject, true);
        addOrRemoveCollisionListener(CollisionType::RESET, this, level, playerCollisionObject, true);
        addOrRemoveCollisionListener(CollisionType::COLLECTABLE, this, level, playerCollisionObject, true);

        for (auto & enemyPair : _enemies)
        {
            EnemyComponent * enemyComponent = enemyPair.second;
            enemyComponent->getTriggerNode()->getCollisionObject()->addCollisionListener(this, playerCollisionObject);
        }
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
    }

    void CollisionHandlerComponent::finalize()
    {
        onLevelUnloaded();
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_forceHandOfGodMessage);
    }

    void CollisionHandlerComponent::collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                        gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                        gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB)
    {
        if(!_waitForPhysicsCleanup && collisionPair.objectA->getNode()->getParent() != collisionPair.objectB->getNode()->getParent())
        {
            if(!onEnemyCollision(type, collisionPair, contactPointA, contactPointB))
            {
                onPlayerCollision(type, collisionPair, contactPointA, contactPointB);
            }
        }
    }

    bool CollisionHandlerComponent::onEnemyCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                                                     gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                                                     gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB)
    {
        if(type == gameplay::PhysicsCollisionObject::CollisionListener::EventType::COLLIDING)
        {
            switch(collisionPair.objectB->getType())
            {
            case gameplay::PhysicsCollisionObject::Type::CHARACTER:
                {
                    EnemyComponent * enemy = gameobjects::GameObject::getGameObject(collisionPair.objectA->getNode()->getParent())->getComponent<EnemyComponent>();

                    if(enemy->getState() != EnemyComponent::State::Dead)
                    {
                        gameplay::PhysicsCharacter * character = static_cast<gameplay::PhysicsCharacter*>(collisionPair.objectB);
                        float const playerVelY = character->getCurrentVelocity().y;
                        if((contactPointB.y >= contactPointA.y && playerVelY != 0) || playerVelY)
                        {
                            enemy->kill();
                            _player->jump(true);
                        }
                        else
                        {
                            getRootParent()->broadcastMessage(_forceHandOfGodMessage);
                        }
                    }

                    return true;
                }
                break;
            case gameplay::PhysicsCollisionObject::Type::RIGID_BODY:
            case gameplay::PhysicsCollisionObject::Type::GHOST_OBJECT:
            case gameplay::PhysicsCollisionObject::Type::VEHICLE:
            case gameplay::PhysicsCollisionObject::Type::VEHICLE_WHEEL:
            case gameplay::PhysicsCollisionObject::Type::NONE:
                break;
            default:
                PLATFORMER_ASSERTFAIL("Unhandled PhysicsCollisionObject type %d", collisionPair.objectB->getType());
            }


        }

        return false;
    }

    bool CollisionHandlerComponent::onPlayerCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                        gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                        gameplay::Vector3 const &, gameplay::Vector3 const &)
    {
        if(collisionPair.objectA == _player->getCharacterNode()->getCollisionObject() &&
            collisionPair.objectB->getType() == gameplay::PhysicsCollisionObject::Type::GHOST_OBJECT)
        {
            if(TerrainInfo * terrainInfo = TerrainInfo::getTerrainInfo(collisionPair.objectB->getNode()))
            {
                bool const isColliding = type == gameplay::PhysicsCollisionObject::CollisionListener::EventType::COLLIDING;

                if(terrainInfo->_CollisionType == CollisionType::LADDER)
                {
                    if (isColliding)
                    {
                        _player->setClimpingSnapPositionX(collisionPair.objectB->getNode()->getTranslationX());
                        ++_playerClimbingTerrainRefCount;
                    }
                    else
                    {
                        --_playerClimbingTerrainRefCount;
                    }
                    
                    _playerClimbingTerrainRefCount = MATH_CLAMP(_playerClimbingTerrainRefCount, 0, std::numeric_limits<int>::max());
                    _player->setClimbingEnabled(_playerClimbingTerrainRefCount > 0);
                    return true;
                }
                else if(terrainInfo->_CollisionType == CollisionType::RESET && isColliding)
                {
                    getRootParent()->broadcastMessage(_forceHandOfGodMessage);
                    return true;
                }
                else if(terrainInfo->_CollisionType == CollisionType::COLLECTABLE)
                {
                    LevelComponent * level = getParent()->getComponent<LevelComponent>();
                    level->consumeCollectable(collisionPair.objectB->getNode());
                    return true;
                }
            }
        }

        return false;
    }
}
