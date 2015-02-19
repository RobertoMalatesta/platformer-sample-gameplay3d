#ifndef GAME_PLAYER_COMPONENT_H
#define GAME_PLAYER_COMPONENT_H

#include "Component.h"

namespace gameplay
{
    class Properties;
}

namespace game
{
    class CameraControlComponent;
    class PlayerInputComponent;
    class PlayerHandOfGodComponent;
    class SpriteAnimationComponent;

    /**
     * Creates a players physics and animation defined in sibling components and updates
     * its local state upon receiving user input.
     *
     * @script{ignore}
    */
    class PlayerComponent : public gameobjects::Component
    {
    public:
        /** @script{ignore} */
        struct State
        {
            enum Enum
            {
                Idle,
                Walking,
                Cowering,
                Jumping,
                Climbing,
                Swimming
            };
        };

        /** @script{ignore} */
        struct MovementDirection
        {
            enum Enum
            {
                None        = 0,
                Left        = 1 << 0,
                Right       = 1 << 1,
                Down        = 1 << 2,
                Up          = 1 << 3,

                EnumCount,

                Horizontal = Left | Right,
                Vertical = Up | Down
            };
        };

        /** @script{ignore} */
        struct JumpSource
        {
            enum Enum
            {
                Input,
                EnemyCollision,
            };
        };

        explicit PlayerComponent();
        ~PlayerComponent();

        virtual void onStart() override;
        virtual void finalize() override;
        void update(float elapsedTime);
        virtual void readProperties(gameplay::Properties & properties) override;
        void forEachAnimation(std::function <bool(State::Enum, SpriteAnimationComponent *)> func);

        State::Enum getState() const;
        gameplay::Vector2 getPosition() const;
        SpriteAnimationComponent * getCurrentAnimation();
        gameplay::Node * getCharacterNode() const;
        void setSwimmingEnabled(bool enabled);
        void setClimbingEnabled(bool enabled);
        void setClimpingSnapPositionX(float posX);
        void setMovementEnabled(MovementDirection::Enum direction, bool enabled, float scale = 1.0f);
        void jump(JumpSource::Enum source, float scale = 1.0f);
        bool isLeftFacing() const;
        void reset(gameplay::Vector2 const position);
    private:
        PlayerComponent(PlayerComponent const &);

        PlayerInputComponent * _playerInputComponent;
        PlayerHandOfGodComponent * _playerHandOfGodComponent;
        gameplay::Node * _characterNode;
        gameplay::Node * _characterNormalNode;
        std::map<State::Enum, SpriteAnimationComponent*> _animations;
        State::Enum _state;
        bool _isLeftFacing;
        bool _climbingEnabled;
        bool _swimmingEnabled;
        MovementDirection::Enum _movementDirection;
        float _movementScale;
        State::Enum _previousState;
        float _movementSpeed;
        float _swimSpeedScale;
        float _jumpHeight;
        float _climbingSnapPositionX;

        std::string _idleAnimComponentId;
        std::string _walkAnimComponentId;
        std::string _cowerAnimComponentId;
        std::string _jumpAnimComponentId;
        std::string _normalCharacterComponentId;
        std::string _climbingCharacterComponentId;
        std::string _swimmingCharacterComponentId;

        gameobjects::Message * _jumpMessage;
        CameraControlComponent * _camera;
    };
}

#endif
