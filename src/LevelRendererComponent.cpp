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
        , _levelLoadedOnce(false)
        , _player(nullptr)
        , _playerInput(nullptr)
        , _level(nullptr)
        , _tileBatch(nullptr)
        , _cameraControl(nullptr)
        , _parallaxSpritebatch(nullptr)
        , _interactablesSpritebatch(nullptr)
        , _pixelSpritebatch(nullptr)
        , _interactablesSpritesheet(nullptr)
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

    gameplay::Rectangle getSafeDrawRect(gameplay::Rectangle const & src, float paddingX = 0.5f, float paddingY = 0.5f)
    {
        return gameplay::Rectangle(src.x + paddingX, src.y + paddingY, src.width - (paddingX * 2), src.height - (paddingY * 2));
    }

    void LevelRendererComponent::onLevelLoaded()
    {
        std::vector<gameplay::SpriteBatch *> uninitialisedSpriteBatches;

        _level = getRootParent()->getComponentInChildren<LevelComponent>();
        _level->addRef();
        _tileBatch = gameplay::SpriteBatch::create(_level->getTexturePath().c_str());
        _tileBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
        _tileBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
        uninitialisedSpriteBatches.push_back(_tileBatch);
        _player = _level->getParent()->getComponentInChildren<PlayerComponent>();
        _playerInput = _player->getParent()->getComponent<PlayerInputComponent>();
        _player->addRef();
        _playerInput->addRef();
        _cameraControl = getRootParent()->getComponentInChildren<CameraControlComponent>();
        _cameraControl->addRef();
        _cameraControl->setBoundary(gameplay::Rectangle(0,
                                                        -(_level->getTileHeight() * _level->getHeight()) * PLATFORMER_UNIT_SCALAR,
                                                        (_level->getTileWidth() * _level->getWidth())  * PLATFORMER_UNIT_SCALAR,
                                                        (_level->getTileHeight() * _level->getHeight()) * PLATFORMER_UNIT_SCALAR));

        _player->forEachAnimation([this, &uninitialisedSpriteBatches](PlayerComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
        {
            SpriteSheet * animSheet = SpriteSheet::create(animation->getSpriteSheetPath());
             gameplay::SpriteBatch * spriteBatch = gameplay::SpriteBatch::create(animSheet->getTexture());
            spriteBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
            spriteBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
            _playerAnimationBatches[state] = spriteBatch;
            uninitialisedSpriteBatches.push_back(spriteBatch);
            SAFE_RELEASE(animSheet);
            return false;
        });

        std::map<std::string, gameplay::SpriteBatch *> enemyuninitialisedSpriteBatches;
        std::vector<EnemyComponent *> enemies;
        _level->getParent()->getComponentsInChildren(enemies);

        for(EnemyComponent * enemy  : enemies)
        {
            enemy->addRef();

            enemy->forEachAnimation([this, &enemyuninitialisedSpriteBatches, &enemy, &uninitialisedSpriteBatches](EnemyComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
            {
                SpriteSheet * animSheet = SpriteSheet::create(animation->getSpriteSheetPath());
                gameplay::SpriteBatch * spriteBatch = nullptr;

                auto enemyBatchItr = enemyuninitialisedSpriteBatches.find(animation->getSpriteSheetPath());

                if(enemyBatchItr != enemyuninitialisedSpriteBatches.end())
                {
                    spriteBatch = enemyBatchItr->second;
                }
                else
                {
                    spriteBatch = gameplay::SpriteBatch::create(animSheet->getTexture());
                    spriteBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
                    spriteBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
                    enemyuninitialisedSpriteBatches[animation->getSpriteSheetPath()] = spriteBatch;
                    uninitialisedSpriteBatches.push_back(spriteBatch);
                }

                _enemyAnimationBatches[enemy][state] = spriteBatch;
                SAFE_RELEASE(animSheet);
                return false;
            });
        }

        if (!_levelLoadedOnce)
        {
            uninitialisedSpriteBatches.push_back(_parallaxSpritebatch);


            for (ParallaxLayer & layer : _parallaxLayers)
            {
                layer._dst.y += (_level->getHeight() * _level->getTileHeight()) - layer._src.height;
            }

            _interactablesSpritesheet = SpriteSheet::create("res/spritesheets/interactables.ss");
            _interactablesSpritebatch = gameplay::SpriteBatch::create(_interactablesSpritesheet->getTexture());
            uninitialisedSpriteBatches.push_back(_interactablesSpritebatch);
        }

        _level->forEachCachedNode(CollisionType::COLLISION_DYNAMIC, [this](gameplay::Node * node)
        {
            node->addRef();
            bool const isBoulder = node->getCollisionObject()->getShapeType() == gameplay::PhysicsCollisionShape::SHAPE_SPHERE;
            _dynamicCollisionNodes.emplace_back(node, getSafeDrawRect(isBoulder ? _interactablesSpritesheet->getSprite("boulder")->_src : 
                _interactablesSpritesheet->getSprite("crate")->_src));
        });

        // The first call to draw will perform some lazy initialisation in Effect::Bind
        for (gameplay::SpriteBatch * spriteBatch : uninitialisedSpriteBatches)
        {
            spriteBatch->start();
            spriteBatch->draw(gameplay::Rectangle(), gameplay::Rectangle());
            spriteBatch->finish();
        }

        _levelLoaded = true;
        _levelLoadedOnce = true;

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

        for (auto & nodePair : _dynamicCollisionNodes)
        {
            SAFE_RELEASE(nodePair.first);
        }

        _dynamicCollisionNodes.clear();
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
        _pixelSpritebatch = createSinglePixelSpritebatch();
    }

    void LevelRendererComponent::finalize()
    {
        SAFE_DELETE(_pixelSpritebatch);
        SAFE_DELETE(_parallaxSpritebatch);
        SAFE_DELETE(_interactablesSpritebatch);
        SAFE_RELEASE(_interactablesSpritesheet);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_splashScreenFadeMessage);
        onLevelUnloaded();
    }

    void LevelRendererComponent::readProperties(gameplay::Properties & properties)
    {
        while (gameplay::Properties * ns = properties.getNextNamespace())
        {
            if (strcmp(ns->getNamespace(), "parallax") == 0)
            {
                SpriteSheet * spritesheet = SpriteSheet::create(ns->getString("spritesheet"));
                ns->getVector4("fill", &_parallaxFillColor);
                ns->getVector2("offset", &_parallaxOffset);
                _parallaxSpritebatch = gameplay::SpriteBatch::create(spritesheet->getTexture());
                _parallaxSpritebatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::REPEAT, gameplay::Texture::Wrap::CLAMP);
                _parallaxSpritebatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);

                while (gameplay::Properties * childNs = ns->getNextNamespace())
                {
                    if (strcmp(childNs->getNamespace(), "layer") == 0)
                    {
                        ParallaxLayer layer;
                        layer._src = spritesheet->getSprite(childNs->getString("id"))->_src;
                        layer._dst = layer._src;
                        gameplay::Vector2 offset;
                        childNs->getVector2("offset", &offset);
                        offset *= -1.0f;
                        layer._dst.x = offset.x - _parallaxOffset.x;
                        layer._dst.y = offset.y - _parallaxOffset.y;
                        layer._speed = childNs->getFloat("speed");
                        _parallaxLayers.push_back(layer);
                    }
                }

                SAFE_RELEASE(spritesheet);
            }
        }
    }

    void LevelRendererComponent::render(float)
    {
        if(_levelLoaded && gameplay::Game::getInstance()->getConfig()->getBool("debug_enabled_level_rendering"))
        {
            // Set the screen to the colour of the sky
            gameplay::Game::getInstance()->clear(gameplay::Game::ClearFlags::CLEAR_COLOR_DEPTH, gameplay::Vector4::fromColor(SKY_COLOR), 1.0f, 0);

            gameplay::Rectangle const & screenDimensions = gameplay::Game::getInstance()->getViewport();
            gameplay::Matrix spriteBatchProjection = _cameraControl->getViewProjectionMatrix();
            spriteBatchProjection.rotateX(MATH_DEG_TO_RAD(180));
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * PLATFORMER_UNIT_SCALAR);
            spriteBatchProjection.scale(unitToPixelScale, unitToPixelScale, 0);

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

            // Draw the parallax background
            float const layerWidth = spriteScreenDimensions.width;
            float const layerPosX = spriteCameraPostion.x - (layerWidth / 2);

            gameplay::Rectangle parallaxFill(layerPosX,
                                                   spriteLevelBounds.y + _parallaxOffset.y,
                                                   layerWidth,
                                                   (spriteLevelBounds.y + (spriteCameraZoomScale)) - spriteCameraPostion.y + (spriteScreenDimensions.height));

            if(parallaxFill.intersects(spriteViewport))
            {
                _pixelSpritebatch->setProjectionMatrix(spriteBatchProjection);
                _pixelSpritebatch->start();
                parallaxFill.y *= -1.0f;
                _pixelSpritebatch->draw(parallaxFill, gameplay::Rectangle(), _parallaxFillColor);
                _pixelSpritebatch->finish();
            }

            bool parallaxLayerDrawn = false;

            for(auto itr = _parallaxLayers.rbegin(); itr != _parallaxLayers.rend(); ++itr)
            {
                ParallaxLayer & layer = *itr;
                layer._dst.width = layerWidth;
                layer._src.width = layerWidth;
                layer._dst.x = layerPosX;
                layer._src.x = (layerPosX + spriteCameraPostion.x) * layer._speed;
                
                gameplay::Rectangle layerVisibilityTest = layer._dst;
                layerVisibilityTest.y += layerVisibilityTest.height;
                layerVisibilityTest.y *= -1.0f;

                if (layerVisibilityTest.intersects(spriteViewport))
                {
                    if (!parallaxLayerDrawn)
                    {
                        _parallaxSpritebatch->setProjectionMatrix(spriteBatchProjection);
                        _parallaxSpritebatch->start();
                        parallaxLayerDrawn = true;
                    }

                    _parallaxSpritebatch->draw(layer._dst, getSafeDrawRect(layer._src, 0, 0.5f));
                }
            }

            if (parallaxLayerDrawn)
            {
                _parallaxSpritebatch->finish();
            }

            if(spriteLevelBounds.intersects(spriteViewport))
            {
                // Draw the tiles
               _tileBatch->setProjectionMatrix(spriteBatchProjection);
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
                            _tileBatch->draw(gameplay::Rectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight),
                                getSafeDrawRect(gameplay::Rectangle(tileX, tileY, tileWidth, tileHeight)));
                        }
                    }
                }


                _tileBatch->finish();
            }

            bool interactableDrawn = false;

            for (auto & nodePair : _dynamicCollisionNodes)
            {
                gameplay::Node * dynamicCollisionNode = nodePair.first;
                bool const isBoulder = dynamicCollisionNode->getCollisionObject()->getShapeType() == gameplay::PhysicsCollisionShape::SHAPE_SPHERE;
                gameplay::Rectangle dst;
                dst.width = dynamicCollisionNode->getScaleX() / PLATFORMER_UNIT_SCALAR;
                dst.height = dynamicCollisionNode->getScaleY() / PLATFORMER_UNIT_SCALAR;
                dst.x = dynamicCollisionNode->getTranslationX() / PLATFORMER_UNIT_SCALAR - (dst.width / 2);
                dst.y = dynamicCollisionNode->getTranslationY() / PLATFORMER_UNIT_SCALAR + (dst.height / 2);
                dst.y -= dst.height;

                if (dst.intersects(spriteViewport))
                {
                    dst.y += dst.height;
                    dst.y *= -1.0f;

                    if (!interactableDrawn)
                    {
                        _interactablesSpritebatch->setProjectionMatrix(spriteBatchProjection);
                        _interactablesSpritebatch->start();
                        interactableDrawn = true;
                    }

                    gameplay::Quaternion const & q = dynamicCollisionNode->getRotation();
                    float const rotation = -static_cast<float>(atan2f(2.0f * q.x * q.y + 2.0f * q.z * q.w, 1.0f - 2.0f * ((q.y * q.y) + (q.z * q.z))));
                    _interactablesSpritebatch->draw(gameplay::Vector3(dst.x, dst.y, 0), // dst
                        nodePair.second,
                        gameplay::Vector2(dst.width, dst.height),
                        gameplay::Vector4::one(),
                        (gameplay::Vector2::one() / 2),
                        rotation);
                }
            }

            if (interactableDrawn)
            {
                _interactablesSpritebatch->finish();
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

    void LevelRendererComponent::update(float elapsedTime)
    {
        for (ParallaxLayer & layer : _parallaxLayers)
        {
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
            spriteBatch->draw(drawTarget._dst, getSafeDrawRect(drawTarget._src), drawTarget._scale);

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
        gameplay::Rectangle const & screenDimensions = gameplay::Game::getInstance()->getViewport();

        if(gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_character_positions"))
        {
            font->start();

            font->drawText("",0,0, gameplay::Vector4::one());

            gameplay::Matrix spriteBatchProjection = _cameraControl->getViewProjectionMatrix();
            spriteBatchProjection.rotateX(MATH_DEG_TO_RAD(180));
            float const spriteCameraZoomScale = (1.0f / PLATFORMER_UNIT_SCALAR) * _cameraControl->getZoom();
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * PLATFORMER_UNIT_SCALAR * spriteCameraZoomScale);
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


        if(gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_camera_target"))
        {
            gameplay::Matrix spriteBatchProjection = _cameraControl->getViewProjectionMatrix();
            spriteBatchProjection.rotateX(MATH_DEG_TO_RAD(180));
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * PLATFORMER_UNIT_SCALAR);
            spriteBatchProjection.scale(unitToPixelScale, unitToPixelScale, 0);
            _pixelSpritebatch->setProjectionMatrix(spriteBatchProjection);

            _pixelSpritebatch->start();

            float const dimensions = 10.0f;
            gameplay::Vector4 targetColour;
            gameplay::Game::getInstance()->getConfig()->getVector4("debug_camera_target_colour", &targetColour);
            gameplay::Rectangle targetBounds(_cameraControl->getTargetPosition().x / PLATFORMER_UNIT_SCALAR,
                                          -_cameraControl->getTargetPosition().y / PLATFORMER_UNIT_SCALAR, dimensions, dimensions);
            _pixelSpritebatch->draw(targetBounds, gameplay::Rectangle(1,1), targetColour);

            _pixelSpritebatch->finish();
        }
    }
#endif
}
