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
        virtual void readProperties(gameplay::Properties & properties) override;
    private:
        class CharacterRenderer
        {
        public:
            explicit CharacterRenderer();
            void start();
            void finish();
            void render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                                 gameplay::Matrix const & spriteBatchProjection, SpriteAnimationComponent::Flip::Enum orientation,
                                 gameplay::Vector2 const & position, gameplay::Rectangle const & viewport, float alpha = 1.0f);
        private:
            gameplay::SpriteBatch * _previousSpritebatch;
            bool _started;
        };

        struct ParallaxLayer
        {
            float _speed;
            gameplay::Rectangle _dst;
            gameplay::Rectangle _src;
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
        bool _levelLoadedOnce;
        gameplay::SpriteBatch * _pixelSpritebatch;
        SpriteSheet * _interactablesSpritesheet;
        std::vector<ParallaxLayer> _parallaxLayers;
        gameplay::SpriteBatch * _parallaxSpritebatch;
        gameplay::SpriteBatch * _interactablesSpritebatch;
        gameplay::SpriteBatch * _collectablesSpritebatch;
        gameplay::Vector4 _parallaxFillColor;
        gameplay::Vector2 _parallaxOffset;
        std::vector<std::pair<gameplay::Node *, gameplay::Rectangle>> _dynamicCollisionNodes;
    };
}

#endif
