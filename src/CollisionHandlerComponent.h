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
    class LevelComponent;
    class PlayerComponent;

    /**
     * Handles collision events between characters and the terrain
     *
     * @script{ignore}
    */
    class CollisionHandlerComponent : public gameobjects::Component, public gameplay::PhysicsCollisionObject::CollisionListener
    {
    public:
        explicit CollisionHandlerComponent();
        ~CollisionHandlerComponent();

        virtual bool onMessageReceived(gameobjects::Message * message, int messageType) override;
        virtual void update(float elapsedTime) override;
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void collisionEvent(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                            gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                            gameplay::Vector3 const &, gameplay::Vector3 const &) override;
    private:
        CollisionHandlerComponent(CollisionHandlerComponent const &);

        void onLevelLoaded();
        void onLevelUnloaded();
        void addPlayerCollisionListeners(gameplay::PhysicsCollisionObject * playerCollisionObject);

        /**
         * Handles collision events between an enemy (contact A) and the player (contact B)
         */
        bool onEnemyCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                              gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                              gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB);

        /**
         * Handles collision events between the player (contact A) and the terrain (contact B)
         */
        bool onPlayerCollision(gameplay::PhysicsCollisionObject::CollisionListener::EventType type,
                            gameplay::PhysicsCollisionObject::CollisionPair const & collisionPair,
                            gameplay::Vector3 const & contactPointA, gameplay::Vector3 const & contactPointB);

        std::map<gameplay::PhysicsCollisionObject *, EnemyComponent *> _enemies;
        PlayerComponent * _player;
        LevelComponent * _level;
        std::set<gameplay::Node *> _playerCharacterNodes;
        gameobjects::Message * _forceHandOfGodMessage;
        int _playerClimbingTerrainRefCount;
        int _playerSwimmingRefCount;
        int _framesSinceLevelReloaded;
        bool _waitForPhysicsCleanup;
    };
}

#endif
