#ifndef PLATFORMER_INPUT_COMPONENT_H
#define PLATFORMER_INPUT_COMPONENT_H

#include "Component.h"
#include "MessagesInput.h"
#include "PlayerComponent.h"

namespace gameplay
{
    class Gamepad;
}

namespace platformer
{
    class CameraControlComponent;

    /**
     * Applies keyboard and gamepad input to a sibling PlayerComponent
     *
     * @script{ignore}
    */
    class PlayerInputComponent : public gameobjects::Component
    {
    public:
        explicit PlayerInputComponent();
        ~PlayerInputComponent();
        gameplay::Form * getGamepadForm() const;
    protected:
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void update(float) override;
        virtual void onStart() override;
        virtual void onMessageReceived(gameplay::AIMessage * message);
    private:
        struct GamepadButtons
        {
            enum Enum
            {
                Jump,

                EnumCount
            };
        };

        PlayerInputComponent(PlayerInputComponent const &);
        bool isGamepadButtonPressed(GamepadButtons::Enum button) const;
        bool isGamepadButtonReleased(GamepadButtons::Enum button) const;
        float calculateCameraZoomStep(bool increase) const;

        void onKeyboardInput(KeyMessage keyMessage);
        void onMouseInput(MouseMessage mouseMessage);
        void onPinchInput(PinchMessage pinchMessage);

        CameraControlComponent * _camera;
        PlayerComponent * _player;
        gameplay::Gamepad * _gamePad;
        std::array<bool, GamepadButtons::EnumCount> _gamepadButtonState;
        std::array<bool, GamepadButtons::EnumCount> _previousGamepadButtonState;
        std::array<int, GamepadButtons::EnumCount> _gamepadButtonMapping;
        std::array<gameplay::Vector2, PlayerComponent::MovementDirection::EnumCount> _joystickMovementDirections;
        gameplay::Vector2 _previousJoystickValue;
    };
}

#endif
