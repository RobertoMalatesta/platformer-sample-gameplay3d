#include "LevelRendererComponent.h"

#include "CameraControlComponent.h"
#include "CollisionObjectComponent.h"
#include "Common.h"
#include "EnemyComponent.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "LevelComponent.h"
#include "Messages.h"
#include "MessagesPlatformer.h"
#include "PlayerComponent.h"
#include "PlayerInputComponent.h"
#include "SpriteSheet.h"

namespace platformer
{
    LevelRendererComponent::LevelRendererComponent()
        : _levelLoaded(false)
        , _player(nullptr)
        , _playerInput(nullptr)
        , _level(nullptr)
        , _tileBatch(nullptr)
        , _cameraControl(nullptr)
    {
    }

    LevelRendererComponent::~LevelRendererComponent()
    {
    }

    void LevelRendererComponent::onMessageReceived(gameplay::AIMessage * message)
    {
        switch (message->getId())
        {
        case(Messages::Type::LevelLoaded):
            onLevelUnloaded();
            onLevelLoaded();
            break;
        case(Messages::Type::LevelUnloaded):
            onLevelUnloaded();
            break;
        }
    }

    void LevelRendererComponent::onLevelLoaded()
    {
        _level = getRootParent()->getComponentInChildren<LevelComponent>();
        _level->addRef();
        _tileBatch = gameplay::SpriteBatch::create(_level->getTexturePath().c_str());
        _tileBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
        _tileBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
        _player = _level->getParent()->getComponentInChildren<PlayerComponent>();
        _playerInput = _player->getParent()->getComponent<PlayerInputComponent>();
        _player->addRef();
        _playerInput->addRef();
        _cameraControl = getRootParent()->getComponentInChildren<CameraControlComponent>();
        _cameraControl->addRef();

        _player->forEachAnimation([this](PlayerComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
        {
            SpriteSheet * animSheet = SpriteSheet::create(animation->getSpriteSheetPath());
             gameplay::SpriteBatch * spriteBatch = gameplay::SpriteBatch::create(animSheet->getTexture());
            spriteBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
            spriteBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
            _playerAnimationBatches[state] = spriteBatch;
            SAFE_RELEASE(animSheet);
            return false;
        });

        std::map<std::string, gameplay::SpriteBatch *> enemySpriteBatches;
        std::vector<EnemyComponent *> enemies;
        _level->getParent()->getComponentsInChildren(enemies);

        for(EnemyComponent * enemy  : enemies)
        {
            enemy->addRef();

            enemy->forEachAnimation([this, &enemySpriteBatches, &enemy](EnemyComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
            {
                SpriteSheet * animSheet = SpriteSheet::create(animation->getSpriteSheetPath());
                gameplay::SpriteBatch * spriteBatch = nullptr;

                auto enemyBatchItr = enemySpriteBatches.find(animation->getSpriteSheetPath());

                if(enemyBatchItr != enemySpriteBatches.end())
                {
                    spriteBatch = enemyBatchItr->second;
                }
                else
                {
                    spriteBatch = gameplay::SpriteBatch::create(animSheet->getTexture());
                    spriteBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
                    spriteBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
                    enemySpriteBatches[animation->getSpriteSheetPath()] = spriteBatch;
                }

                _enemyAnimationBatches[enemy][state] = spriteBatch;
                SAFE_RELEASE(animSheet);
                return false;
            });
        }

        _levelLoaded = true;

        float const fadeOutDuration = 1.0f;
        PlatformerSplashScreenChangeRequestMessage::setMessage(_splashScreenFadeMessage, fadeOutDuration, false);
        this->getRootParent()->broadcastMessage(_splashScreenFadeMessage);
    }

    void LevelRendererComponent::onLevelUnloaded()
    {
        SAFE_RELEASE(_cameraControl);
        SAFE_RELEASE(_level);
        SAFE_RELEASE(_player);
        SAFE_RELEASE(_playerInput);

        SAFE_DELETE(_tileBatch);

        for (auto & playerAnimBatchPairItr : _playerAnimationBatches)
        {
            SAFE_DELETE(playerAnimBatchPairItr.second);
        }

        std::set<gameplay::SpriteBatch *> uniqueEnemyBatches;

        for (auto & enemyAnimPairItr : _enemyAnimationBatches)
        {
            EnemyComponent * enemy = enemyAnimPairItr.first;
            SAFE_RELEASE(enemy);

            for (auto & enemyAnimBatchPairItr : enemyAnimPairItr.second)
            {
                uniqueEnemyBatches.insert(enemyAnimBatchPairItr.second);
            }
        }

        for(gameplay::SpriteBatch * spriteBatch : uniqueEnemyBatches)
        {
            SAFE_DELETE(spriteBatch);
        }

        _playerAnimationBatches.clear();
        _enemyAnimationBatches.clear();
        _levelLoaded = false;

        if(_splashScreenFadeMessage)
        {
            float const fadeInDuration = 0.0f;
            PlatformerSplashScreenChangeRequestMessage::setMessage(_splashScreenFadeMessage, fadeInDuration, true);
            getRootParent()->broadcastMessage(_splashScreenFadeMessage);
        }
    }

    void LevelRendererComponent::initialize()
    {
        _splashScreenFadeMessage = PlatformerSplashScreenChangeRequestMessage::create();
    }

    void LevelRendererComponent::finalize()
    {
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_splashScreenFadeMessage);
        onLevelUnloaded();
    }

    void LevelRendererComponent::render(float)
    {
        if(_levelLoaded)
        {
            // Set the screen to the colour of the sky
            gameplay::Game::getInstance()->clear(gameplay::Game::ClearFlags::CLEAR_COLOR_DEPTH, gameplay::Vector4::fromColor(SKY_COLOR), 1.0f, 0);

            gameplay::Rectangle const & screenDimensions = gameplay::Game::getInstance()->getViewport();
            gameplay::Matrix spriteBatchProjection = _cameraControl->getViewProjectionMatrix();
            spriteBatchProjection.rotateX(MATH_DEG_TO_RAD(180));
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * PLATFORMER_UNIT_SCALAR);
            spriteBatchProjection.scale(unitToPixelScale, unitToPixelScale, 0);
            _tileBatch->setProjectionMatrix(spriteBatchProjection);

            int const tileWidth = _level->getTileWidth();
            int const tileHeight = _level->getTileHeight();

            gameplay::Vector2 const spriteCameraPostion(_cameraControl->getPosition().x / PLATFORMER_UNIT_SCALAR, _cameraControl->getPosition().y / PLATFORMER_UNIT_SCALAR);
            gameplay::Rectangle const spriteLevelBounds = gameplay::Rectangle(0, -(tileHeight * _level->getHeight()), tileWidth * _level->getWidth(), tileHeight * _level->getHeight());
            gameplay::Rectangle spriteScreenDimensions = screenDimensions;
            float const spriteCameraZoomScale = (1.0f / PLATFORMER_UNIT_SCALAR) * _cameraControl->getZoom();

#ifndef _FINAL
            if(gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_zoom_draw_culling"))
#endif
            {
                spriteScreenDimensions.width *= spriteCameraZoomScale;
                spriteScreenDimensions.height *= spriteCameraZoomScale;
            }

            gameplay::Rectangle const spriteViewport(spriteCameraPostion.x - (spriteScreenDimensions.width / 2),
                                               spriteCameraPostion.y - (spriteScreenDimensions.height / 2),
                                               spriteScreenDimensions.width, spriteScreenDimensions.height);

            if(spriteLevelBounds.intersects(spriteViewport))
            {
                // Draw the level
               _tileBatch->start();

                int const minX = spriteViewport.x > 0 ? MATH_CLAMP(ceil((spriteViewport.x - tileWidth) / tileWidth), 0, _level->getWidth() - 1) : 0;
                int const maxX = MATH_CLAMP(minX + ((spriteViewport.width + (tileWidth * 2)) / tileWidth), 0, _level->getWidth());
                float const spriteViewPortY = spriteViewport.y + spriteViewport.height;
                int const minY = spriteViewPortY < 0 ? MATH_CLAMP(fabs(ceil((spriteViewPortY + tileHeight) / tileHeight)), 0, _level->getHeight()) : 0;
                int const maxY = MATH_CLAMP(minY + ((spriteViewport.height + (tileHeight * 3)) / tileHeight), 0, _level->getHeight());

                int const numSpritesX = _tileBatch->getSampler()->getTexture()->getWidth() / tileWidth;

                for (int y = minY; y < maxY; ++y)
                {
                    for (int x = minX; x < maxX; ++x)
                    {
                        int tile = _level->getTile(x, y);

                        if (tile != LevelComponent::EMPTY_TILE)
                        {
                            int const tileIndex = tile - 1;
                            int const tileX = (tileIndex % numSpritesX) * tileWidth;
                            int const tileY = (tileIndex / numSpritesX) * tileHeight;
                            static float const fpPrecisionPadding = 0.5f;
                            _tileBatch->draw(gameplay::Rectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight),
                                gameplay::Rectangle(tileX + fpPrecisionPadding, tileY + fpPrecisionPadding, tileWidth - (fpPrecisionPadding * 2), tileHeight - (fpPrecisionPadding * 2)));
                        }
                    }
                }

                _tileBatch->finish();
            }

            _characterRenderer.start();

            // Draw the player
            _characterRenderer.render(_player->getCurrentAnimation(),
                            _playerAnimationBatches[_player->getState()], spriteBatchProjection,
                            _player->IsLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                            _player->getPosition(), spriteViewport);

            // Draw the enemies
            for (auto & enemyAnimPairItr : _enemyAnimationBatches)
            {
                EnemyComponent * enemy = enemyAnimPairItr.first;
                std::map<int, gameplay::SpriteBatch *> & enemyBatches = enemyAnimPairItr.second;
                _characterRenderer.render(enemy->getCurrentAnimation(),
                                enemyBatches[enemy->getState()], spriteBatchProjection,
                                enemy->IsLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                                enemy->getPosition(), spriteViewport);
            }

            _characterRenderer.finish();

            // Draw the virtual controls
            if(gameplay::Form * gamepadForm = _playerInput->getGamepadForm())
            {
                gamepadForm->draw();
            }
        }
    }

    LevelRendererComponent::CharacterRenderer::CharacterRenderer()
        : _started(false)
        , _previousSpritebatch(nullptr)
    {
    }

    void LevelRendererComponent::CharacterRenderer::start()
    {
        PLATFORMER_ASSERT(!_started, "Start called before Finish");
        _started = true;
    }

    void LevelRendererComponent::CharacterRenderer::finish()
    {
        PLATFORMER_ASSERT(_started, "Finsh called before Start");
        _started = false;

        if(_previousSpritebatch)
        {
            _previousSpritebatch->finish();
            _previousSpritebatch = nullptr;
        }
    }

    void LevelRendererComponent::CharacterRenderer::render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                         gameplay::Matrix const & spriteBatchProjection, SpriteAnimationComponent::Flip::Enum orientation,
                         gameplay::Vector2 const & position, gameplay::Rectangle const & viewport)
    {
        SpriteAnimationComponent::DrawTarget drawTarget = animation->getDrawTarget(gameplay::Vector2::one(), 0.0f, orientation);
        gameplay::Vector2 playerDrawPosition = position / PLATFORMER_UNIT_SCALAR;
        playerDrawPosition.x -= fabs(drawTarget._scale.x / 2);
        gameplay::Rectangle const playerBounds(playerDrawPosition.x, playerDrawPosition.y - fabs(drawTarget._scale.y / 2), fabs(drawTarget._scale.x), fabs(drawTarget._scale.y));
        playerDrawPosition.y += fabs(drawTarget._scale.y / 2);
        playerDrawPosition.y *= -1.0f;
        drawTarget._dst.x += playerDrawPosition.x;
        drawTarget._dst.y += playerDrawPosition.y;

        if(playerBounds.intersects(viewport))
        {
            if(_previousSpritebatch != spriteBatch)
            {
                if(_previousSpritebatch)
                {
                    _previousSpritebatch->finish();
                }

                spriteBatch->start();
            }

            spriteBatch->setProjectionMatrix(spriteBatchProjection);
            spriteBatch->draw(drawTarget._dst, drawTarget._src, drawTarget._scale);

            _previousSpritebatch = spriteBatch;
        }
    }

#ifndef _FINAL
    void renderCharacterPositionDebug(gameplay::Vector2 const & position, gameplay::Vector2 const & renderPosition, gameplay::Font * font)
    {
        std::array<char, 32> buffer;
        sprintf(&buffer[0], "{%.2f, %.2f}", position.x, position.y);
        unsigned int width, height = 0;
        font->measureText(&buffer[0], font->getSize(PLATFORMER_FONT_SIZE_LARGE_INDEX), &width, &height);
        font->drawText(&buffer[0], renderPosition.x - (width / 4), -renderPosition.y, gameplay::Vector4(1,0,0,1));
    }

    void LevelRendererComponent::renderDebug(float, gameplay::Font * font)
    {
        if(gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_character_positions"))
        {
            font->start();

            font->drawText("",0,0, gameplay::Vector4::one());

            gameplay::Matrix spriteBatchProjection = _cameraControl->getViewProjectionMatrix();
            spriteBatchProjection.rotateX(MATH_DEG_TO_RAD(180));
            float const unitToPixelScale = 1.0f / (gameplay::Game::getInstance()->getHeight()) / (gameplay::Game::getInstance()->getHeight() * PLATFORMER_UNIT_SCALAR)
                    * font->getSize(PLATFORMER_FONT_SIZE_LARGE_INDEX);
            spriteBatchProjection.scale(unitToPixelScale, unitToPixelScale, 0);
            gameplay::SpriteBatch * spriteBatch = font->getSpriteBatch(font->getSize());
            spriteBatch->setProjectionMatrix(spriteBatchProjection);

            renderCharacterPositionDebug(_player->getPosition(), _player->getPosition() / unitToPixelScale, font);

            for (auto & enemyAnimPairItr : _enemyAnimationBatches)
            {
                EnemyComponent * enemy = enemyAnimPairItr.first;
                renderCharacterPositionDebug(enemy->getPosition(), enemy->getPosition() / unitToPixelScale, font);
            }

            font->finish();
        }
    }
#endif
}
