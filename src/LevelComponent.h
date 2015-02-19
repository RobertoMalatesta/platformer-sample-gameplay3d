#ifndef GAME_LEVEL_RENDERER_COMPONENT_H
#define GAME_LEVEL_RENDERER_COMPONENT_H

#include "Component.h"
#include "LevelLoaderComponent.h"
#include "SpriteAnimationComponent.h"

namespace game
{
    class CameraControlComponent;
    class CollisionHandlerComponent;
    class EnemyComponent;
    class LevelLoaderComponent;
    class PlayerComponent;
    class SpriteSheet;

    /**
     * Handles update and render for a level loaded by LevelLoaderComponent
     *
     * @script{ignore}
    */
    class LevelComponent : public gameobjects::Component
    {
    public:
        explicit LevelComponent();
        ~LevelComponent();
    protected:
        virtual void initialize() override;
        virtual void finalize() override;
        virtual bool onMessageReceived(gameobjects::Message * message, int messageType) override;
        virtual void readProperties(gameplay::Properties & properties) override;
    private:
        class CharacterRenderer
        {
        public:
            explicit CharacterRenderer();
            void start();
            void finish();
            bool render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                                 gameplay::Matrix const & spriteBatchProjection, SpriteAnimationComponent::Flip::Enum orientation,
                                 gameplay::Vector2 const & position, gameplay::Rectangle const & viewport, float alpha = 1.0f, gameplay::Rectangle * dstOut = nullptr);
        private:
            gameplay::SpriteBatch * _previousSpritebatch;
            bool _started;
        };

        struct ParallaxLayer
        {
            float _speed;
            gameplay::Rectangle _dst;
            gameplay::Rectangle _src;
            gameplay::Vector2 _offset;
            bool _cameraIndependent;
        };

        LevelComponent(LevelComponent const &);

        void onLevelLoaded();
        void onLevelUnloaded();
        void updateAndRender(float elapsedTime);
        void updateAndRenderCharacters(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport, float elapsedTime);
        void updateAndRenderBackground(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, float elapsedTime);
        void renderTileMap(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderInteractables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderCollectables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport);
        void renderWater(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, float elapsedTime);
        float getWaterTimeUniform() const;


#ifndef _FINAL
        void renderDebug(gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport);
#endif

        PlayerComponent * _player;
        LevelLoaderComponent * _level;
        CollisionHandlerComponent * _levelCollisionHandler;
        std::map<int, gameplay::SpriteBatch *> _playerAnimationBatches;
        std::map<EnemyComponent *, std::map<int, gameplay::SpriteBatch *>> _enemyAnimationBatches;
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
        gameplay::SpriteBatch * _waterSpritebatch;
        gameplay::Vector4 _parallaxFillColor;
        gameplay::Vector2 _parallaxOffset;
        std::vector<std::pair<gameplay::Node *, gameplay::Rectangle>> _dynamicCollisionNodes;
        std::vector<LevelLoaderComponent::Collectable *> _collectables;
        std::vector<gameplay::Rectangle> _waterBounds;
        gameplay::FrameBuffer * _frameBuffer;
        gameplay::SpriteBatch * _pauseSpriteBatch;
        float _waterUniformTimer;
    };
}

#endif
