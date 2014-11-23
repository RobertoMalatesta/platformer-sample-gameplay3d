#ifndef PLATFORMER_SPRITE_ANIMATION_COMPONENT_H
#define PLATFORMER_SPRITE_ANIMATION_COMPONENT_H

#include "Component.h"
#include "SpriteSheet.h"

namespace gameplay
{
    class Properties;
}

namespace platformer
{
    /**
     * Creates a configurable sprite animation, rendering not handled by this component.
     *
     * @script{ignore}
    */
    class SpriteAnimationComponent : public gameobjects::Component
    {
    public:
        /** @script{ignore} */
        struct PlaybackState
        {
            enum Enum
            {
                Stopped,
                Playing,
                Paused
            };
        };

        /** @script{ignore} */
        struct Flip
        {
            enum Enum
            {
                None = 1 << 0,
                Vertical = 1 << 1,
                Horizontal = 1 << 2
            };
        };

        /** @script{ignore} */
        struct DrawTarget
        {
            gameplay::Vector2 _scale;
            gameplay::Vector3 _dst;
            gameplay::Rectangle _src;
        };

        explicit SpriteAnimationComponent();
        ~SpriteAnimationComponent();

        virtual void initialize() override;
        virtual void readProperties(gameplay::Properties & properties) override;
        virtual void update(float elapsedTime) override;

        void play();
        void pause();
        void stop();
        void setSpeed(float speed);

        PlaybackState::Enum getState() const;
        gameplay::Rectangle const & getCurrentSpriteSrc() const;
        std::string const & getSpriteSheetPath() const;
        std::string const & getSpritePrefix() const;
        DrawTarget getDrawTarget(gameplay::Vector2 const & scale, float rotationRadians, Flip::Enum flip);
        float getSpeed() const;
        bool getIsLooping() const;
    private:
        SpriteAnimationComponent(SpriteAnimationComponent const &);

        float _elapsed;
        int _frameCount;
        int _frameIndex;
        float _speed;
        float _fps;
        bool _autoStart;
        bool _loop;
        gameplay::Vector2 _maxSpriteDimensions;
        PlaybackState::Enum _playbackState;
        std::string _spritePrefix;
        std::string _spriteSheetPath;
        std::vector<Sprite> _sprites;
        std::vector<std::string> _spriteNames;
    };
}

#endif
