#ifndef PLATFORMER_LEVEL_RENDERER_COMPONENT_H
#define PLATFORMER_LEVEL_RENDERER_COMPONENT_H

#include "Component.h"
#include "SpriteAnimationComponent.h"

namespace platformer
{
    class CameraControlComponent;
    class EnemyComponent;
    class LevelComponent;
    class PlayerComponent;
    class PlayerInputComponent;
    class SpriteSheet;

    /**
     * Renders all visual data defined in a LevelComponent and updates the camera
     *
     * @script{ignore}
    */
    class LevelRendererComponent : public gameobjects::Component
    {
    public:
        explicit LevelRendererComponent();
        ~LevelRendererComponent();
        static unsigned int const SKY_COLOR = 0xD0F4F7FF;
    protected:
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void render(float);
#ifndef _FINAL
        virtual void renderDebug(float, gameplay::Font * font);
#endif
        virtual void onMessageReceived(gameplay::AIMessage * message) override;
    private:
        class CharacterRenderer
        {
        public:
            explicit CharacterRenderer();
            void start();
            void finish();
            void render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                                 gameplay::Matrix const & spriteBatchProjection, SpriteAnimationComponent::Flip::Enum orientation,
                                 gameplay::Vector2 const & position, gameplay::Rectangle const & viewport);
        private:
            gameplay::SpriteBatch * _previousSpritebatch;
            bool _started;
        };

        LevelRendererComponent(LevelRendererComponent const &);

        void onLevelLoaded();
        void onLevelUnloaded();

        PlayerComponent * _player;
        PlayerInputComponent * _playerInput;
        LevelComponent * _level;
        std::map<int, gameplay::SpriteBatch *> _playerAnimationBatches;
        std::map<EnemyComponent *, std::map<int, gameplay::SpriteBatch *>> _enemyAnimationBatches;
        gameplay::AIMessage * _splashScreenFadeMessage;
        gameplay::SpriteBatch * _tileBatch;
        CameraControlComponent * _cameraControl;
        CharacterRenderer _characterRenderer;
        bool _levelLoaded;

#ifndef _FINAL
        gameplay::SpriteBatch * _pixelSpritebatch;
#endif
    };
}

#endif
