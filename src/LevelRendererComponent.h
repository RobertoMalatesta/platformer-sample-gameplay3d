#ifndef GAME_LEVEL_RENDERER_COMPONENT_H
#define GAME_LEVEL_RENDERER_COMPONENT_H

#include "Component.h"
#include "LevelComponent.h"
#include "SpriteAnimationComponent.h"

namespace game
{
    class CameraControlComponent;
    class EnemyComponent;
    class LevelComponent;
    class PlayerComponent;
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
        virtual void finalize() override;
        virtual void update(float elaspedTime);
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
            gameplay::Vector2 _offset;
            bool _cameraIndependent;
        };

        LevelRendererComponent(LevelRendererComponent const &);

        void onLevelLoaded();
        void onLevelUnloaded();
        void render();
        void renderBackground(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderTileMap(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderInteractables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderCollectables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderCharacters(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        void renderWater(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport);
        float getWaterTimeUniform() const;


#ifndef _FINAL
        void renderDebug();
#endif

        PlayerComponent * _player;
        LevelComponent * _level;
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
        std::vector<LevelComponent::Collectable *> _collectables;
        std::vector<gameplay::Rectangle> _waterBounds;
        float _waterUniformTimer;
    };
}

#endif
