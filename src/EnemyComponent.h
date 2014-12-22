#ifndef PLATFORMER_ENEMY_COMPONENT_H
#define PLATFORMER_ENEMY_COMPONENT_H

#include "Component.h"

namespace gameplay
{
    class Properties;
}

namespace platformer
{
    class CollisionObjectComponent;
    class SpriteAnimationComponent;

    /**
     * A simple enemy behaviour that walks horizontally along a surface until it reaches an edge,
     * after which, it will head in the opposite direciton.
     *
     * @script{ignore}
    */
    class EnemyComponent : public gameobjects::Component
    {
    public:
        /** @script{ignore} */
        struct State
        {
            enum Enum
            {
                Walking
            };
        };

        explicit EnemyComponent();
        ~EnemyComponent();

        void onStart() override;
        void finalize() override;
        void update(float elapsedTime) override;
        void readProperties(gameplay::Properties & properties) override;
        void forEachAnimation(std::function <bool(State::Enum, SpriteAnimationComponent *)> func);
        void setHorizontalConstraints(float minX, float maxX);
        void onTerrainCollision();
        State::Enum getState() const;
        gameplay::Vector2 getPosition() const;
        SpriteAnimationComponent * getCurrentAnimation();
        gameplay::Node * getTriggerNode() const;

        bool IsLeftFacing() const;
    private:
        EnemyComponent(EnemyComponent const &);

        gameplay::Node * _triggerNode;
        std::map<State::Enum, SpriteAnimationComponent*> _animations;
        bool _isLeftFacing;
        State::Enum _state;
        float _movementSpeed;
        float _minX;
        float _maxX;
        std::string _walkAnimComponentId;
        std::string _triggerComponentId;
    };
}

#endif
