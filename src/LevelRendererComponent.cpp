#include "LevelRendererComponent.h"

#include "CameraControlComponent.h"
#include "CollisionObjectComponent.h"
#include "Common.h"
#include "EnemyComponent.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "Messages.h"
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
        , _collectablesSpritebatch(nullptr)
        , _pixelSpritebatch(nullptr)
        , _interactablesSpritesheet(nullptr)
        , _waterSpritebatch(nullptr)
        , _waterUniformTimer(0.0f)
    {
    }

    LevelRendererComponent::~LevelRendererComponent()
    {
    }

    void LevelRendererComponent::onMessageReceived(gameobjects::GameObjectMessage message, int messageType)
    {
        switch (messageType)
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

    gameplay::Rectangle getRenderDestination(gameplay::Rectangle const & worldDestination)
    {
        gameplay::Rectangle result;
        result.width = worldDestination.width / PLATFORMER_UNIT_SCALAR;
        result.height = worldDestination.height / PLATFORMER_UNIT_SCALAR;
        result.x = worldDestination.x / PLATFORMER_UNIT_SCALAR;
        result.y = -worldDestination.y / PLATFORMER_UNIT_SCALAR - result.height;
        return result;
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
        _cameraControl->setBoundary(gameplay::Rectangle(_level->getTileWidth() * 2.0f * PLATFORMER_UNIT_SCALAR, 0,
            (_level->getTileWidth() * _level->getWidth() - (_level->getTileWidth() * 4.0f))  * PLATFORMER_UNIT_SCALAR,
            std::numeric_limits<float>::max()));

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
            _pixelSpritebatch = createSinglePixelSpritebatch();
            uninitialisedSpriteBatches.push_back(_pixelSpritebatch);

            uninitialisedSpriteBatches.push_back(_parallaxSpritebatch);

            _interactablesSpritesheet = SpriteSheet::create("res/spritesheets/interactables.ss");
            _interactablesSpritebatch = gameplay::SpriteBatch::create(_interactablesSpritesheet->getTexture());
            uninitialisedSpriteBatches.push_back(_interactablesSpritebatch);


            gameplay::Effect* waterEffect = gameplay::Effect::createFromFile("res/shaders/sprite.vert", "res/shaders/water.frag");
            _waterSpritebatch = gameplay::SpriteBatch::create("@res/textures/water", waterEffect);
            _waterSpritebatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::REPEAT, gameplay::Texture::Wrap::CLAMP);
            SAFE_RELEASE(waterEffect);
            uninitialisedSpriteBatches.push_back(_waterSpritebatch);
            gameplay::Material* waterMaterial = _waterSpritebatch->getMaterial();
            gameplay::Texture::Sampler* noiseSampler = gameplay::Texture::Sampler::create("res/textures/water-noise.png");
            waterMaterial->getParameter("u_texture_noise")->setValue(noiseSampler);
            SAFE_RELEASE(noiseSampler);
            waterMaterial->getParameter("u_time")->bindValue(this, &LevelRendererComponent::getWaterTimeUniform);

            SpriteSheet * collectablesSpriteSheet = SpriteSheet::create("res/spritesheets/collectables.ss");
            _collectablesSpritebatch = gameplay::SpriteBatch::create(collectablesSpriteSheet->getTexture());
            SAFE_RELEASE(collectablesSpriteSheet);
            uninitialisedSpriteBatches.push_back(_collectablesSpritebatch);
        }



        _level->getCollectables(_collectables);

        _level->forEachCachedNode(CollisionType::COLLISION_DYNAMIC, [this](gameplay::Node * node)
        {
            node->addRef();
            bool const isBoulder = node->getCollisionObject()->getShapeType() == gameplay::PhysicsCollisionShape::SHAPE_SPHERE;
            _dynamicCollisionNodes.emplace_back(node, getSafeDrawRect(isBoulder ? _interactablesSpritesheet->getSprite("boulder")->_src : 
                _interactablesSpritesheet->getSprite("crate")->_src));
        });

        _level->forEachCachedNode(CollisionType::BRIDGE, [this](gameplay::Node * node)
        {
            node->addRef();
            _dynamicCollisionNodes.emplace_back(node, _interactablesSpritesheet->getSprite("bridge")->_src);
        });

        _waterUniformTimer = 0.0f;

        _level->forEachCachedNode(CollisionType::WATER, [this](gameplay::Node * node)
        {
            gameplay::Rectangle bounds;
            bounds.width = node->getScaleX();
            bounds.height = node->getScaleY();
            bounds.x = node->getTranslationX() - bounds.width / 2.0f;
            bounds.y = node->getTranslationY() - bounds.height / 2.0f;
            // Scale the boundary height to add the area that was removed to make room for waves in the texture (95px of 512px)
            float const textureScale = 1.18f;
            bounds.height *= textureScale;
            _waterBounds.push_back(bounds);
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
        RequestSplashScreenFadeMessage::setMessage(_splashScreenFadeMessage, fadeOutDuration, false, true);
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
        _waterBounds.clear();
        _collectables.clear();
        _levelLoaded = false;

        if(_splashScreenFadeMessage)
        {
            float const fadeInDuration = 0.0f;
            RequestSplashScreenFadeMessage::setMessage(_splashScreenFadeMessage, fadeInDuration, true, true);
            getRootParent()->broadcastMessage(_splashScreenFadeMessage);
        }
    }

    void LevelRendererComponent::initialize()
    {
        _splashScreenFadeMessage = RequestSplashScreenFadeMessage::create();
    }

    void LevelRendererComponent::finalize()
    {
        SAFE_DELETE(_pixelSpritebatch);
        SAFE_DELETE(_parallaxSpritebatch);
        SAFE_DELETE(_interactablesSpritebatch);
        SAFE_DELETE(_collectablesSpritebatch);
        SAFE_DELETE(_waterSpritebatch);
        SAFE_RELEASE(_interactablesSpritesheet);
        GAMEOBJECTS_DELETE_MESSAGE(_splashScreenFadeMessage);
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
                _parallaxOffset *= PLATFORMER_UNIT_SCALAR;
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
                        layer._dst.width *= PLATFORMER_UNIT_SCALAR;
                        layer._dst.height *= PLATFORMER_UNIT_SCALAR;
                        childNs->getVector2("offset", &layer._offset);
                        layer._offset *= PLATFORMER_UNIT_SCALAR;
                        layer._dst.y = layer._offset.y + _parallaxOffset.y;
                        layer._src.x = layer._offset.x + _parallaxOffset.x;
                        layer._speed = childNs->getFloat("speed");
                        layer._cameraIndependent = childNs->getBool("camera_independent");
                        _parallaxLayers.push_back(layer);
                    }
                }

                SAFE_RELEASE(spritesheet);
            }
        }
    }

    void LevelRendererComponent::update(float elapsedTime)
    {
        float const dt = elapsedTime / 1000.0f;
        _waterUniformTimer += dt;

        for (auto itr = _parallaxLayers.rbegin(); itr != _parallaxLayers.rend(); ++itr)
        {
            ParallaxLayer & layer = *itr;

            if (layer._cameraIndependent)
            {
                layer._src.x += (layer._speed * dt) / PLATFORMER_UNIT_SCALAR;
            }
        }
    }

    void LevelRendererComponent::renderBackground(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        // Clear the screen to the colour of the sky
        gameplay::Game::getInstance()->clear(gameplay::Game::ClearFlags::CLEAR_COLOR_DEPTH, gameplay::Vector4::fromColor(SKY_COLOR), 1.0f, 0);

        // Draw the solid colour parallax fill layer
        float const layerWidth = viewport.width;
        float const layerPosX = viewport.x;

        gameplay::Rectangle parallaxFillLayer(layerPosX, 0.0f, layerWidth, _parallaxOffset.y);

        if(parallaxFillLayer.intersects(viewport))
        {
            _pixelSpritebatch->setProjectionMatrix(projection);
            _pixelSpritebatch->start();
            _pixelSpritebatch->draw(getRenderDestination(parallaxFillLayer), gameplay::Rectangle(), _parallaxFillColor);
            _pixelSpritebatch->finish();
        }

        // Draw the parallax texture layers
        bool parallaxLayerDrawn = false;

        for(auto itr = _parallaxLayers.rbegin(); itr != _parallaxLayers.rend(); ++itr)
        {
            ParallaxLayer & layer = *itr;
            layer._dst.width = layerWidth;
            layer._dst.x = layerPosX;

            if (layer._dst.intersects(viewport))
            {
                if (!parallaxLayerDrawn)
                {
                    _parallaxSpritebatch->setProjectionMatrix(projection);
                    _parallaxSpritebatch->start();
                    parallaxLayerDrawn = true;
                }

                if (!layer._cameraIndependent)
                {
                    layer._src.x = ((layerPosX + layer._offset.x + _parallaxOffset.x + viewport.x) * layer._speed) / PLATFORMER_UNIT_SCALAR;
                }

                layer._src.width = layer._dst.width / PLATFORMER_UNIT_SCALAR;

                _parallaxSpritebatch->draw(getRenderDestination(layer._dst), getSafeDrawRect(layer._src, 0, 0.5f));
            }
        }

        if (parallaxLayerDrawn)
        {
            _parallaxSpritebatch->finish();
        }
    }

    void LevelRendererComponent::renderTileMap(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        gameplay::Rectangle const levelArea((_level->getTileWidth() * _level->getWidth()) * PLATFORMER_UNIT_SCALAR,
            (_level->getTileHeight() * _level->getHeight()) * PLATFORMER_UNIT_SCALAR);

        if(levelArea.intersects(viewport))
        {
            // Draw only the tiles the player can see
            int const tileWidth = _level->getTileWidth();
            int const tileHeight = _level->getTileHeight();
            float const renderedOffsetY = _level->getHeight() * tileHeight;
            gameplay::Rectangle renderedViewport = getRenderDestination(viewport);
            renderedViewport.y *= -1.0f;
            renderedViewport.y -= renderedOffsetY;

            int const minXUnClamped = ceil((renderedViewport.x - tileWidth) / tileWidth);
            int const minX = MATH_CLAMP(minXUnClamped, 0, _level->getWidth());
            int const maxX = MATH_CLAMP(minXUnClamped + ((renderedViewport.width + tileWidth * 2.0f) / tileWidth), 0, _level->getWidth());

            int const minYUnClamped = -ceil((renderedViewport.y) / tileHeight);
            int const minY = MATH_CLAMP(minYUnClamped, 0, _level->getHeight());
            int const maxY = MATH_CLAMP(minYUnClamped + ((renderedViewport.height + (tileHeight * 2.0f)) / tileHeight), 0, _level->getHeight());

            int const numSpritesX = _tileBatch->getSampler()->getTexture()->getWidth() / tileWidth;
            bool tilesRendered = false;

            for (int y = minY; y < maxY; ++y)
            {
                for (int x = minX; x < maxX; ++x)
                {
                    int tile = _level->getTile(x, y);

                    if (tile != LevelComponent::EMPTY_TILE)
                    {
                        if(!tilesRendered)
                        {
                            _tileBatch->setProjectionMatrix(projection);
                            _tileBatch->start();
                            tilesRendered = true;
                        }

                        int const tileIndex = tile - 1;
                        int const tileX = (tileIndex % numSpritesX) * tileWidth;
                        int const tileY = (tileIndex / numSpritesX) * tileHeight;
                        _tileBatch->draw(gameplay::Rectangle(x * tileWidth, (y * tileHeight) - renderedOffsetY, tileWidth, tileHeight),
                            getSafeDrawRect(gameplay::Rectangle(tileX, tileY, tileWidth, tileHeight)));
                    }
                }
            }

            if(tilesRendered)
            {
                _tileBatch->finish();
            }
        }
    }

    void LevelRendererComponent::renderInteractables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        bool interactableDrawn = false;

        // Draw dynamic collision (crates, boulders etc)
        for (auto & nodePair : _dynamicCollisionNodes)
        {
            gameplay::Node * dynamicCollisionNode = nodePair.first;
            gameplay::Rectangle dst;
            dst.width = dynamicCollisionNode->getScaleX();
            dst.height = dynamicCollisionNode->getScaleY();
            dst.x = dynamicCollisionNode->getTranslationX() - (dst.width / 2);
            dst.y = dynamicCollisionNode->getTranslationY() - (dst.height / 2);

            gameplay::Rectangle maxBounds = dst;

            // Extend non-spherical shapes for viewport intersection test, prevents shapes being culled when still visible during rotation
            if(dynamicCollisionNode->getCollisionObject()->getShapeType() != gameplay::PhysicsCollisionShape::Type::SHAPE_SPHERE)
            {
                maxBounds.width = gameplay::Vector2(dst.width, dst.height).length();
                maxBounds.height = maxBounds.width;
                maxBounds.x = dynamicCollisionNode->getTranslationX() - (maxBounds.width / 2);
                maxBounds.y = dynamicCollisionNode->getTranslationY() - (maxBounds.height / 2);
            }

            if (maxBounds.intersects(viewport))
            {
                if (!interactableDrawn)
                {
                    _interactablesSpritebatch->setProjectionMatrix(projection);
                    _interactablesSpritebatch->start();
                    interactableDrawn = true;
                }

                gameplay::Quaternion const & q = dynamicCollisionNode->getRotation();
                float const rotation = -static_cast<float>(atan2f(2.0f * q.x * q.y + 2.0f * q.z * q.w, 1.0f - 2.0f * ((q.y * q.y) + (q.z * q.z))));
                gameplay::Rectangle const renderDst = getRenderDestination(dst);
                _interactablesSpritebatch->draw(gameplay::Vector3(renderDst.x, renderDst.y, 0),
                    nodePair.second,
                    gameplay::Vector2(renderDst.width, renderDst.height),
                    gameplay::Vector4::one(),
                    (gameplay::Vector2::one() / 2),
                    rotation);
            }
        }

        if (interactableDrawn)
        {
            _interactablesSpritebatch->finish();
        }
    }

    void LevelRendererComponent::renderCollectables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        bool collectableDrawn = false;

        for(LevelComponent::Collectable * collectable : _collectables)
        {
            if (collectable->_active)
            {
                if (!collectableDrawn)
                {
                    _collectablesSpritebatch->setProjectionMatrix(projection);
                    _collectablesSpritebatch->start();
                    collectableDrawn = true;
                }

                gameplay::Rectangle dst;
                dst.width = collectable->_node->getScaleX();
                dst.height = collectable->_node->getScaleY();
                dst.x = collectable->_node->getTranslationX() - dst.width / 2;
                dst.y = collectable->_node->getTranslationY() - dst.height / 2;

                collectable->_visible = dst.intersects(viewport);

                if (collectable->_visible)
                {
                    _collectablesSpritebatch->draw(getRenderDestination(dst), getSafeDrawRect(collectable->_src));
                }
            }
        }

        if(collectableDrawn)
        {
            _collectablesSpritebatch->finish();
        }
    }

    void LevelRendererComponent::renderCharacters(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        _characterRenderer.start();

        // Draw the enemies
        for (auto & enemyAnimPairItr : _enemyAnimationBatches)
        {
            EnemyComponent * enemy = enemyAnimPairItr.first;
            float const alpha = enemy->getAlpha();

            if(alpha > 0.0f)
            {
                std::map<int, gameplay::SpriteBatch *> & enemyBatches = enemyAnimPairItr.second;
                _characterRenderer.render(enemy->getCurrentAnimation(),
                                enemyBatches[enemy->getState()], projection,
                                enemy->isLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                                enemy->getPosition(), viewport, alpha);
            }
        }

        // Draw the player
        _characterRenderer.render(_player->getCurrentAnimation(),
                        _playerAnimationBatches[_player->getState()], projection,
                        _player->isLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                        _player->getPosition(), viewport);

        _characterRenderer.finish();
    }

    void LevelRendererComponent::renderWater(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        if (!_waterBounds.empty())
        {
            bool waterDrawn = false;

            for (gameplay::Rectangle const & dst : _waterBounds)
            {
                if(dst.intersects((viewport)))
                {
                    if(!waterDrawn)
                    {
                        _waterSpritebatch->setProjectionMatrix(projection);
                        _waterSpritebatch->start();
                        waterDrawn = true;
                    }

                    gameplay::Rectangle src;
                    src.width = _waterSpritebatch->getSampler()->getTexture()->getWidth();
                    src.height = _waterSpritebatch->getSampler()->getTexture()->getHeight();
                    _waterSpritebatch->draw(getRenderDestination(dst), getSafeDrawRect(src));
                }
            }

            if(waterDrawn)
            {
                _waterSpritebatch->finish();
            }
            else
            {
                _waterUniformTimer = 0;
            }
        }
    }

    bool LevelRendererComponent::render(float)
    {
        bool renderingEnabled = _levelLoaded;
#ifndef _FINAL
        renderingEnabled &= gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_level_rendering");
#endif
        if(renderingEnabled)
        {
            float const zoomScale = (1.0f / PLATFORMER_UNIT_SCALAR) * _cameraControl->getZoom();
            gameplay::Rectangle viewport;
            viewport.width = gameplay::Game::getInstance()->getViewport().width * PLATFORMER_UNIT_SCALAR,
            viewport.height = gameplay::Game::getInstance()->getViewport().height * PLATFORMER_UNIT_SCALAR;

#ifndef _FINAL
            if(gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_zoom_draw_culling"))
#endif
            {
                viewport.width *= zoomScale;
                viewport.height *= zoomScale;
            }

            viewport.x = _cameraControl->getPosition().x - (viewport.width / 2.0f);
            viewport.y = _cameraControl->getPosition().y - (viewport.height / 2.0f);

            gameplay::Matrix projection = _cameraControl->getViewProjectionMatrix();
            projection.rotateX(MATH_DEG_TO_RAD(180));
            projection.scale(PLATFORMER_UNIT_SCALAR, PLATFORMER_UNIT_SCALAR, 0);

            renderBackground(projection, viewport);
            renderTileMap(projection, viewport);
            renderInteractables(projection, viewport);
            renderCollectables(projection, viewport);
            renderCharacters(projection, viewport);
            renderWater(projection, viewport);
        }


        return false;
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

    float LevelRendererComponent::getWaterTimeUniform() const
    {
        return _waterUniformTimer * MATH_PIX2;
    }

    void LevelRendererComponent::CharacterRenderer::render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                         gameplay::Matrix const & projection, SpriteAnimationComponent::Flip::Enum orientation,
                         gameplay::Vector2 const & position, gameplay::Rectangle const & viewport, float alpha)
    {
        SpriteAnimationComponent::DrawTarget drawTarget = animation->getDrawTarget(gameplay::Vector2::one(), 0.0f, orientation);
        gameplay::Rectangle const bounds(position.x - ((fabs(drawTarget._scale.x / 2) * PLATFORMER_UNIT_SCALAR)),
                                                  position.y - ((fabs(drawTarget._scale.y / 2) * PLATFORMER_UNIT_SCALAR)),
                                                  fabs(drawTarget._scale.x) * PLATFORMER_UNIT_SCALAR,
                                                  fabs(drawTarget._scale.y) * PLATFORMER_UNIT_SCALAR);

        if(bounds.intersects(viewport))
        {
            gameplay::Vector2 drawPosition = position / PLATFORMER_UNIT_SCALAR;
            drawPosition.x -= fabs(drawTarget._scale.x / 2);
            drawPosition.y += fabs(drawTarget._scale.y / 2);
            drawPosition.y *= -1.0f;
            drawTarget._dst.x += drawPosition.x;
            drawTarget._dst.y += drawPosition.y;

            if(_previousSpritebatch != spriteBatch)
            {
                if(_previousSpritebatch)
                {
                    _previousSpritebatch->finish();
                }

                spriteBatch->start();
            }

            spriteBatch->setProjectionMatrix(projection);
            spriteBatch->draw(drawTarget._dst, getSafeDrawRect(drawTarget._src), drawTarget._scale, gameplay::Vector4(1.0f, 1.0f, 1.0f, alpha));

            _previousSpritebatch = spriteBatch;
        }
    }

#ifndef _FINAL
    void renderCharacterDataDebug(gameplay::Vector2 * position, gameplay::Vector2 * velocity,
                                  gameplay::Vector2 const & renderPosition, gameplay::Font * font)
    {
        std::array<char, 32> buffer;
        static std::string text;
        text = "";

        if(position)
        {
            sprintf(&buffer[0], " {%.2f, %.2f} ", position->x, position->y);
            text += &buffer[0];
        }

        if(velocity)
        {
            sprintf(&buffer[0], " {%.2f, %.2f} ", velocity->x, velocity->y);
            text += &buffer[0];
        }

        unsigned int width, height = 0;
        font->measureText(text.c_str(), font->getSize(PLATFORMER_FONT_SIZE_LARGE_INDEX), &width, &height);
        font->drawText(text.c_str(), renderPosition.x - (width / 4), -renderPosition.y, gameplay::Vector4(1,0,0,1));
    }

    void LevelRendererComponent::renderDebug(float, gameplay::Font * font)
    {
        gameplay::Rectangle const & screenDimensions = gameplay::Game::getInstance()->getViewport();

        bool const drawPositions = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_character_positions");
        bool const drawPlayerVelocity = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_player_velocity");
        bool const drawCameraTarget = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_camera_target");
        bool const drawViewport = !gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_zoom_draw_culling");

        if(drawPlayerVelocity || drawPositions)
        {
            font->start();

            font->drawText("",0,0, gameplay::Vector4::one());

            gameplay::Matrix projection = _cameraControl->getViewProjectionMatrix();
            projection.rotateX(MATH_DEG_TO_RAD(180));
            float const spriteCameraZoomScale = (1.0f / PLATFORMER_UNIT_SCALAR) * _cameraControl->getZoom();
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * PLATFORMER_UNIT_SCALAR * spriteCameraZoomScale);
            projection.scale(unitToPixelScale, unitToPixelScale, 0);
            gameplay::SpriteBatch * spriteBatch = font->getSpriteBatch(font->getSize());
            spriteBatch->setProjectionMatrix(projection);

            gameplay::PhysicsCharacter * playerCharacter = static_cast<gameplay::PhysicsCharacter*>(_player->getCharacterNode()->getCollisionObject());
            gameplay::Vector2 playerVelocity(playerCharacter->getCurrentVelocity().x, playerCharacter->getCurrentVelocity().y);
            gameplay::Vector2 playerPosition(_player->getPosition());
            renderCharacterDataDebug(drawPositions ? &playerPosition : nullptr, drawPlayerVelocity ? &playerVelocity : nullptr, _player->getPosition() / unitToPixelScale, font);

            if(drawPositions)
            {
                for (auto & enemyAnimPairItr : _enemyAnimationBatches)
                {
                    EnemyComponent * enemy = enemyAnimPairItr.first;
                    gameplay::Vector2 enemyPosition = enemy->getPosition();
                    renderCharacterDataDebug(&enemyPosition, nullptr, enemy->getPosition() / unitToPixelScale, font);
                }
            }

            font->finish();
        }

        gameplay::Matrix projection = _cameraControl->getViewProjectionMatrix();
        projection.rotateX(MATH_DEG_TO_RAD(180));
        projection.scale(PLATFORMER_UNIT_SCALAR, PLATFORMER_UNIT_SCALAR, 0);
        _pixelSpritebatch->setProjectionMatrix(projection);

        if(drawCameraTarget)
        {
            _pixelSpritebatch->start();

            float const dimensions = 10.0f;
            gameplay::Vector4 targetColour;
            gameplay::Game::getInstance()->getConfig()->getVector4("debug_camera_target_colour", &targetColour);
            gameplay::Rectangle targetBounds(_cameraControl->getTargetPosition().x / PLATFORMER_UNIT_SCALAR,
                                          -_cameraControl->getTargetPosition().y / PLATFORMER_UNIT_SCALAR, dimensions, dimensions);
            _pixelSpritebatch->draw(targetBounds, gameplay::Rectangle(1,1), targetColour);

            _pixelSpritebatch->finish();
        }

        if(drawViewport)
        {
            float const zoomScale = (1.0f / PLATFORMER_UNIT_SCALAR) * _cameraControl->getZoom();
            gameplay::Rectangle viewport;
            viewport.width = gameplay::Game::getInstance()->getViewport().width * PLATFORMER_UNIT_SCALAR,
            viewport.height = gameplay::Game::getInstance()->getViewport().height * PLATFORMER_UNIT_SCALAR;
            viewport.x = _cameraControl->getPosition().x - (viewport.width / 2.0f);
            viewport.y = _cameraControl->getPosition().y - (viewport.height / 2.0f);
            _pixelSpritebatch->setProjectionMatrix(projection);
            _pixelSpritebatch->start();
            _pixelSpritebatch->draw(getRenderDestination(viewport), gameplay::Rectangle(), gameplay::Vector4(0,0,0,0.5f));
            _pixelSpritebatch->finish();
        }
    }
#endif
}
