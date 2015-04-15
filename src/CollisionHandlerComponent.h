#ifndef GAME_COLLISION_HANDLER_COMPONENT_H
#define GAME_COLLISION_HANDLER_COMPONENT_H

#include "Component.h"
#include "PhysicsCollisionObject.h"

namespace gameplay
{
    class AIMessage;
}

namespace game
{
    class EnemyComponent;
    class LevelLoaderComponent;
    class PlayerComponent;

    /**
     * Handles collision events between characters and the terrain
     *
     * @script{ignore}
    */
    class CollisionHandlerComponent : public gameobjects::Component
    {
        friend class EnemyCollisionListener;
        friend class PlayerCollisionListener;

    public:
        explicit CollisionHandlerComponent();
        ~CollisionHandlerComponent();

        virtual bool onMessageReceived(gameobjects::Message * message, int messageType) override;
        void update(float elapsedTime);
        virtual void initialize() override;
        virtual void finalize() override;

    private:
        CollisionHandlerComponent(CollisionHandlerComponent const &);

        void onLevelLoaded();
        void onLevelUnloaded();

        /**
         * Handles collision events between an enemy (contact A) and the player (contact B)
         */
        void onEnemyCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                              gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                              gameplay::Vector3 const &, gameplay::Vector3 const &);

        /**
         * Handles collision events between the player (contact A) and the terrain (contact B)
         */
        void onPlayerCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                            gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                            gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB);

        struct EnemyCollisionListener : public gameplay::PhysicsCollisionObject::CollisionListener, gameplay::Ref
        {
            virtual void collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                                gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                                gameplay::Vector3 const &, gameplay::Vector3 const &) override;
            CollisionHandlerComponent * _collisionHandler;
        };

        struct PlayerCollisionListener : public gameplay::PhysicsCollisionObject::CollisionListener, gameplay::Ref
        {
            virtual void collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                                gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                                gameplay::Vector3 const &, gameplay::Vector3 const &) override;
            CollisionHandlerComponent * _collisionHandler;
        };

        EnemyCollisionListener * _enemyCollisionListener;
        PlayerCollisionListener * _playerCollisionListener;
        std::map<gameplay::PhysicsCollisionObject *, EnemyComponent *> _enemies;
        PlayerComponent * _player;
        LevelLoaderComponent * _level;
        gameplay::Node * _playerPhysicsNode;
        gameplay::Node * _playerTriggerNode;
        gameobjects::Message * _forceHandOfGodMessage;
        gameobjects::Message * _enemyKilledMessage;
        int _playerClimbingTerrainRefCount;
        int _playerSwimmingRefCount;
        int _framesSinceLevelReloaded;
        bool _waitForPhysicsCleanup;
    };
}

#endif
