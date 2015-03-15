#include "LevelComponent.h"

#include "CameraControlComponent.h"
#include "CollisionHandlerComponent.h"
#include "Common.h"
#include "EnemyComponent.h"
#include "Game.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "Messages.h"
#include "PhysicsCharacter.h"
#include "PlayerComponent.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "ScreenRenderer.h"
#include "SpriteSheet.h"

namespace game
{
    LevelComponent::LevelComponent()
        : _levelLoaded(false)
        , _levelLoadedOnce(false)
        , _player(nullptr)
        , _level(nullptr)
        , _levelCollisionHandler(nullptr)
        , _tileBatch(nullptr)
        , _cameraControl(nullptr)
        , _parallaxSpritebatch(nullptr)
        , _interactablesSpritebatch(nullptr)
        , _collectablesSpritebatch(nullptr)
        , _pixelSpritebatch(nullptr)
        , _interactablesSpritesheet(nullptr)
        , _waterSpritebatch(nullptr)
        , _frameBuffer(nullptr)
        , _waterUniformTimer(0.0f)
    {
    }

    LevelComponent::~LevelComponent()
    {
    }

    bool LevelComponent::onMessageReceived(gameobjects::Message * message, int messageType)
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
        case(Messages::Type::UpdateAndRenderLevel):
        {
            UpdateAndRenderLevelMessage msg(message);
            updateAndRender(msg._elapsedTime);
        }
            return false;
        }

        return true;
    }

    gameplay::Rectangle getSafeDrawRect(gameplay::Rectangle const & src, float paddingX = 0.5f, float paddingY = 0.5f)
    {
        return gameplay::Rectangle(src.x + paddingX, src.y + paddingY, src.width - (paddingX * 2), src.height - (paddingY * 2));
    }

    gameplay::Rectangle getRenderDestination(gameplay::Rectangle const & worldDestination)
    {
        gameplay::Rectangle result;
        result.width = worldDestination.width / GAME_UNIT_SCALAR;
        result.height = worldDestination.height / GAME_UNIT_SCALAR;
        result.x = worldDestination.x / GAME_UNIT_SCALAR;
        result.y = -worldDestination.y / GAME_UNIT_SCALAR - result.height;
        return result;
    }

    void LevelComponent::onLevelLoaded()
    {
        PERF_SCOPE("LevelComponent::onLevelLoaded");

        std::vector<gameplay::SpriteBatch *> uninitialisedSpriteBatches;

        _tileBatch = gameplay::SpriteBatch::create(_level->getTexturePath().c_str());
        _tileBatch->getSampler()->setFilterMode(gameplay::Texture::Filter::NEAREST, gameplay::Texture::Filter::NEAREST);
        _tileBatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::CLAMP, gameplay::Texture::Wrap::CLAMP);
        uninitialisedSpriteBatches.push_back(_tileBatch);
        _player = _level->getParent()->getComponentInChildren<PlayerComponent>();
        _player->addRef();
        _cameraControl->setBoundary(gameplay::Rectangle(_level->getTileWidth() * 2.0f * GAME_UNIT_SCALAR, 0,
            (_level->getTileWidth() * _level->getWidth() - (_level->getTileWidth() * 2.0f))  * GAME_UNIT_SCALAR,
            std::numeric_limits<float>::max()));

        _player->forEachAnimation([this, &uninitialisedSpriteBatches](PlayerComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
        {
            SpriteSheet * animSheet = ResourceManager::getInstance().getSpriteSheet(animation->getSpriteSheetPath());
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


        _tileMap.resize(_level->getHeight());

        for (auto & horizontalTiles : _tileMap)
        {
            horizontalTiles.resize(_level->getWidth());
        }

        for(int y = 0; y < _level->getHeight(); ++y)
        {
            for(int x = 0; x < _level->getWidth(); ++x)
            {
                _tileMap[y][x].id = _level->getTile(x, y);
                _tileMap[y][x].foreground = false;
            }
        }

        for(EnemyComponent * enemy  : enemies)
        {
            enemy->addRef();

            enemy->forEachAnimation([this, &enemyuninitialisedSpriteBatches, &enemy, &uninitialisedSpriteBatches](EnemyComponent::State::Enum state, SpriteAnimationComponent * animation) -> bool
            {
                SpriteSheet * animSheet = ResourceManager::getInstance().getSpriteSheet(animation->getSpriteSheetPath());
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
            _pixelSpritebatch = ResourceManager::getInstance().createSinglePixelSpritebatch();
            uninitialisedSpriteBatches.push_back(_pixelSpritebatch);

            uninitialisedSpriteBatches.push_back(_parallaxSpritebatch);

            _interactablesSpritesheet = ResourceManager::getInstance().getSpriteSheet("res/spritesheets/interactables.ss");
            _interactablesSpritebatch = gameplay::SpriteBatch::create(_interactablesSpritesheet->getTexture());
            uninitialisedSpriteBatches.push_back(_interactablesSpritebatch);

            gameplay::Effect* waterEffect = gameplay::Effect::createFromFile("res/shaders/sprite.vert", "res/shaders/water.frag");
            _waterSpritebatch = gameplay::SpriteBatch::create("@res/textures/water", waterEffect);
            _waterSpritebatch->getSampler()->setWrapMode(gameplay::Texture::Wrap::REPEAT, gameplay::Texture::Wrap::CLAMP);
            SAFE_RELEASE(waterEffect);
            uninitialisedSpriteBatches.push_back(_waterSpritebatch);
            gameplay::Material* waterMaterial = _waterSpritebatch->getMaterial();
            gameplay::Texture::Sampler* noiseSampler = gameplay::Texture::Sampler::create("@res/textures/water-noise");
            waterMaterial->getParameter("u_texture_noise")->setValue(noiseSampler);
            SAFE_RELEASE(noiseSampler);
            waterMaterial->getParameter("u_time")->bindValue(this, &LevelComponent::getWaterTimeUniform);

            SpriteSheet * collectablesSpriteSheet = ResourceManager::getInstance().getSpriteSheet("res/spritesheets/collectables.ss");
            _collectablesSpritebatch = gameplay::SpriteBatch::create(collectablesSpriteSheet->getTexture());
            SAFE_RELEASE(collectablesSpriteSheet);
            uninitialisedSpriteBatches.push_back(_collectablesSpritebatch);

            _frameBuffer = gameplay::FrameBuffer::create("lr_buffer");
            gameplay::RenderTarget * pauseRenderTarget = gameplay::RenderTarget::create("pause", gameplay::Game::getInstance()->getWidth(), gameplay::Game::getInstance()->getHeight());
            _frameBuffer->setRenderTarget(pauseRenderTarget);
            gameplay::Effect * pauseEffect = gameplay::Effect::createFromFile("res/shaders/sprite.vert", "res/shaders/sepia.frag");
            _pauseSpriteBatch = gameplay::SpriteBatch::create(pauseRenderTarget->getTexture(), pauseEffect);
            SAFE_RELEASE(pauseEffect);
            SAFE_RELEASE(pauseRenderTarget);
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

        int foregroundTileDrawCommandSize = 0;

        _level->forEachCachedNode(CollisionType::WATER, [this, &foregroundTileDrawCommandSize](gameplay::Node * node)
        {
            gameplay::Rectangle bounds;
            bounds.width = node->getScaleX();
            bounds.height = node->getScaleY();
            bounds.x = node->getTranslationX() - bounds.width / 2.0f;
            bounds.y = node->getTranslationY() - bounds.height / 2.0f;
            // Scale the boundary height to add the area that was removed to make room for waves in the texture (95px of 512px)
            float const textureScale = 1.18f;
            bounds.height *= textureScale;

            gameplay::Rectangle waterTileArea;
            waterTileArea.width = (node->getScaleX() / GAME_UNIT_SCALAR) / _level->getTileWidth();
            waterTileArea.height = (node->getScaleY() / GAME_UNIT_SCALAR) / _level->getTileHeight();
            waterTileArea.x = (bounds.x / GAME_UNIT_SCALAR) / _level->getTileWidth();
            waterTileArea.y = _level->getHeight() - (((bounds.y + bounds.height) / GAME_UNIT_SCALAR) / _level->getTileHeight());

            for(int y = waterTileArea.y; y < waterTileArea.y + waterTileArea.height; ++y)
            {
                for(int x = waterTileArea.x; x < waterTileArea.x + waterTileArea.width; ++x)
                {
                    if(_tileMap[y][x].id != LevelLoaderComponent::EMPTY_TILE)
                    {
                        _tileMap[y][x].foreground = true;
                        ++foregroundTileDrawCommandSize;
                    }
                }
            }

            _waterBounds.push_back(bounds);
        });

        _foregroundTileDrawCommands.resize(foregroundTileDrawCommandSize);

        // The first call to draw will perform some lazy initialisation in Effect::Bind
        for (gameplay::SpriteBatch * spriteBatch : uninitialisedSpriteBatches)
        {
            spriteBatch->start();
            spriteBatch->draw(gameplay::Rectangle(), gameplay::Rectangle());
            spriteBatch->finish();
        }

        _levelLoaded = true;
        _levelLoadedOnce = true;

        float const fadeOutDuration = 2.5f;
        ScreenRenderer::getInstance().queueFadeOut(fadeOutDuration);
    }

    void LevelComponent::onLevelUnloaded()
    {
        PERF_SCOPE("LevelComponent::onLevelUnLoaded");

        SAFE_RELEASE(_player);
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
        _tileMap.clear();
        _foregroundTileDrawCommands.clear();

        if(_levelLoaded)
        {
            ScreenRenderer::getInstance().queueFadeToLoadingScreen(0.0f);
        }

        _levelLoaded = false;
    }

    void LevelComponent::initialize()
    {
        _level = getRootParent()->getComponentInChildren<LevelLoaderComponent>();
        _level->addRef();
        _levelCollisionHandler = getRootParent()->getComponentInChildren<CollisionHandlerComponent>();
        _levelCollisionHandler->addRef();
        _cameraControl = getRootParent()->getComponentInChildren<CameraControlComponent>();
        _cameraControl->addRef();
    }

    void LevelComponent::finalize()
    {
        SAFE_RELEASE(_level);
        SAFE_RELEASE(_levelCollisionHandler);
        SAFE_RELEASE(_cameraControl);
        SAFE_DELETE(_pixelSpritebatch);
        SAFE_DELETE(_parallaxSpritebatch);
        SAFE_DELETE(_interactablesSpritebatch);
        SAFE_DELETE(_collectablesSpritebatch);
        SAFE_DELETE(_waterSpritebatch);
        SAFE_DELETE(_pauseSpriteBatch);
        SAFE_RELEASE(_interactablesSpritesheet);
        SAFE_RELEASE(_frameBuffer);
        onLevelUnloaded();
    }

    void LevelComponent::readProperties(gameplay::Properties & properties)
    {
        while (gameplay::Properties * ns = properties.getNextNamespace())
        {
            if (strcmp(ns->getNamespace(), "parallax") == 0)
            {
                SpriteSheet * spritesheet = ResourceManager::getInstance().getSpriteSheet(ns->getString("spritesheet"));
                ns->getVector4("fill", &_parallaxFillColor);
                ns->getVector2("offset", &_parallaxOffset);
                _parallaxOffset *= GAME_UNIT_SCALAR;
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
                        layer._dst.width *= GAME_UNIT_SCALAR;
                        layer._dst.height *= GAME_UNIT_SCALAR;
                        childNs->getVector2("offset", &layer._offset);
                        layer._offset *= GAME_UNIT_SCALAR;
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

    void LevelComponent::updateAndRenderBackground(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, float elapsedTime)
    {
        // Clear the screen to the colour of the sky
        static unsigned int const SKY_COLOR = 0xD0F4F7FF;
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

            if (layer._cameraIndependent && gameplay::Game::getInstance()->getState() == gameplay::Game::State::RUNNING)
            {
                layer._src.x += (layer._speed * (elapsedTime / 1000.0f)) / GAME_UNIT_SCALAR;
            }

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
                    layer._src.x = ((layerPosX + layer._offset.x + _parallaxOffset.x + viewport.x) * layer._speed) / GAME_UNIT_SCALAR;
                }

                layer._src.width = layer._dst.width / GAME_UNIT_SCALAR;

                _parallaxSpritebatch->draw(getRenderDestination(layer._dst), getSafeDrawRect(layer._src, 0, 0.5f));
            }
        }

        if (parallaxLayerDrawn)
        {
            _parallaxSpritebatch->finish();
        }
    }

    int LevelComponent::renderBackgroundTiles(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
    {
        int foregroundTilesCount = 0;
        gameplay::Rectangle const levelArea((_level->getTileWidth() * _level->getWidth()) * GAME_UNIT_SCALAR,
            (_level->getTileHeight() * _level->getHeight()) * GAME_UNIT_SCALAR);

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
                    Tile const & tile =  _tileMap[y][x];

                    if (tile.id != LevelLoaderComponent::EMPTY_TILE)
                    {
                        if(!tilesRendered && !tile.foreground)
                        {
                            _tileBatch->setProjectionMatrix(projection);
                            _tileBatch->start();
                            tilesRendered = true;
                        }

                        int const tileIndex = tile.id - 1;
                        int const tileX = (tileIndex % numSpritesX) * tileWidth;
                        int const tileY = (tileIndex / numSpritesX) * tileHeight;
                        gameplay::Rectangle dst(x * tileWidth, (y * tileHeight) - renderedOffsetY, tileWidth, tileHeight);
                        gameplay::Rectangle src(getSafeDrawRect(gameplay::Rectangle(tileX, tileY, tileWidth, tileHeight)));

                        if(!tile.foreground)
                        {
                            _tileBatch->draw(dst,src);
                        }
                        else
                        {
                            auto & drawCommandPair = _foregroundTileDrawCommands[foregroundTilesCount];
                            drawCommandPair.first = dst;
                            drawCommandPair.second = src;
                            ++foregroundTilesCount;
                        }
                    }
                }
            }

            if(tilesRendered)
            {
                _tileBatch->finish();
            }
        }

        return foregroundTilesCount;
    }

    void LevelComponent::renderForegroundTiles(int tileCount)
    {
        if(tileCount > 0)
        {
            _tileBatch->start();
            for(int i = 0; i < tileCount; ++i)
            {
                auto const & drawCommandPair = _foregroundTileDrawCommands[i];
                _tileBatch->draw(drawCommandPair.first, drawCommandPair.second);
            }
            _tileBatch->finish();
        }
    }

    void LevelComponent::renderInteractables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport)
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

    void LevelComponent::renderCollectables(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport)
    {
        bool collectableDrawn = false;

        for(LevelLoaderComponent::Collectable * collectable : _collectables)
        {
            gameplay::Rectangle dst;
            dst.width = collectable->_node->getScaleX();
            dst.height = collectable->_node->getScaleY();
            dst.x = collectable->_startPosition.x - dst.width / 2;
            dst.y = collectable->_startPosition.y - dst.height / 2;

            if (collectable->_active)
            {
                if (!collectableDrawn)
                {
                    _collectablesSpritebatch->setProjectionMatrix(projection);
                    _collectablesSpritebatch->start();
                    collectableDrawn = true;
                }

                collectable->_visible = dst.intersects(viewport);

                if (collectable->_visible)
                {
                    if(gameplay::Game::getInstance()->getState() != gameplay::Game::State::PAUSED)
                    {
                        float const speed = 5.0f;
                        float const height = collectable->_node->getScaleY() * 0.05f;
                        float bounce = sin((gameplay::Game::getGameTime() / 1000.0f) * speed + (collectable->_node->getTranslationX() + collectable->_node->getTranslationY())) * height;
                        dst.y += bounce;
                    }
                    _collectablesSpritebatch->draw(getRenderDestination(dst), getSafeDrawRect(collectable->_src));
                }
            }

            collectable->_node->getCollisionObject()->setEnabled(collectable->_active && dst.intersects(triggerViewport));
        }

        if(collectableDrawn)
        {
            _collectablesSpritebatch->finish();
        }
    }

    void LevelComponent::updateAndRenderCharacters(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport, float elapsedTime)
    {
        _characterRenderer.start();

        // Enemies
        for (auto & enemyAnimPairItr : _enemyAnimationBatches)
        {
            EnemyComponent * enemy = enemyAnimPairItr.first;
            float const alpha = enemy->getAlpha();

            if(alpha > 0.0f)
            {
                std::map<int, gameplay::SpriteBatch *> & enemyBatches = enemyAnimPairItr.second;
                gameplay::Rectangle dst;
                _characterRenderer.render(enemy->getCurrentAnimation(),
                                enemyBatches[enemy->getState()], projection,
                                enemy->isLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                                enemy->getPosition(), viewport, alpha, &dst);
                enemy->getTriggerNode()->getCollisionObject()->setEnabled(dst.intersects(triggerViewport) && alpha == 1.0f);
                enemy->update(elapsedTime);
            }
        }

        // Player
        _characterRenderer.render(_player->getCurrentAnimation(),
                        _playerAnimationBatches[_player->getState()], projection,
                        _player->isLeftFacing() ? SpriteAnimationComponent::Flip::Horizontal : SpriteAnimationComponent::Flip::None,
                        _player->getRenderPosition(), viewport);

        _characterRenderer.finish();
    }

    void LevelComponent::renderWater(gameplay::Matrix const & projection, gameplay::Rectangle const & viewport, float elapsedTime)
    {
        if (!_waterBounds.empty())
        {
            if(gameplay::Game::getInstance()->getState() == gameplay::Game::State::RUNNING)
            {
                float const dt = elapsedTime / 1000.0f;
                _waterUniformTimer += dt;
            }

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

    void LevelComponent::updateAndRender(float elapsedTime)
    {
        _level->processLoadRequests();

        bool const isPaused = gameplay::Game::getInstance()->getState() == gameplay::Game::State::PAUSED;

        if(!isPaused)
        {
            if(_levelLoaded)
            {
                _player->update(elapsedTime);
                _cameraControl->update(_player->getRenderPosition(), elapsedTime);
            }

            _levelCollisionHandler->update(elapsedTime);
        }

        bool renderingEnabled = _levelLoaded;
#ifndef _FINAL
        renderingEnabled &= gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_level_rendering");
#endif
        if(renderingEnabled)
        {
            gameplay::FrameBuffer * previousFrameBuffer = nullptr;

            if(isPaused)
            {
                previousFrameBuffer = _frameBuffer->bind();
            }

            float const zoomScale = (1.0f / GAME_UNIT_SCALAR) * _cameraControl->getZoom();
            gameplay::Rectangle viewport;
            viewport.width = gameplay::Game::getInstance()->getViewport().width * GAME_UNIT_SCALAR,
            viewport.height = gameplay::Game::getInstance()->getViewport().height * GAME_UNIT_SCALAR;

#ifndef _FINAL
            if(gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_zoom_draw_culling"))
#endif
            {
                viewport.width *= zoomScale;
                viewport.height *= zoomScale;
            }

            viewport.x = _cameraControl->getPosition().x - (viewport.width / 2.0f);
            viewport.y = _cameraControl->getPosition().y - (viewport.height / 2.0f);

            gameplay::Rectangle triggerViewport;
            float const triggerViewportScale = (1.0f / GAME_UNIT_SCALAR) * _cameraControl->getMinZoom();
            triggerViewport.width = viewport.width * triggerViewportScale;
            triggerViewport.height = viewport.height * triggerViewportScale;
            triggerViewport.x = _player->getPosition().x - (triggerViewport.width / 2.0f);
            triggerViewport.y = _player->getPosition().y - (triggerViewport.height / 2.0f);

            gameplay::Matrix projection = _cameraControl->getViewProjectionMatrix();
            projection.rotateX(MATH_DEG_TO_RAD(180));
            projection.scale(GAME_UNIT_SCALAR, GAME_UNIT_SCALAR, 0);

            updateAndRenderBackground(projection, viewport, elapsedTime);
            int const foregroundTileCount = renderBackgroundTiles(projection, viewport);
            renderInteractables(projection, viewport);
            renderCollectables(projection, viewport, triggerViewport);
            updateAndRenderCharacters(projection, viewport, triggerViewport, elapsedTime);
            renderWater(projection, viewport, elapsedTime);
            renderForegroundTiles(foregroundTileCount);

            if(previousFrameBuffer)
            {
                previousFrameBuffer->bind();

                _pauseSpriteBatch->start();
                _pauseSpriteBatch->draw(gameplay::Game::getInstance()->getViewport(), gameplay::Game::getInstance()->getViewport());
                _pauseSpriteBatch->finish();
            }
#ifndef _FINAL
            renderDebug(viewport, triggerViewport);
#endif
        }
    }

    LevelComponent::CharacterRenderer::CharacterRenderer()
        : _started(false)
        , _previousSpritebatch(nullptr)
    {
    }

    void LevelComponent::CharacterRenderer::start()
    {
        GAME_ASSERT(!_started, "Start called before Finish");
        _started = true;
    }

    void LevelComponent::CharacterRenderer::finish()
    {
        GAME_ASSERT(_started, "Finsh called before Start");
        _started = false;

        if(_previousSpritebatch)
        {
            _previousSpritebatch->finish();
            _previousSpritebatch = nullptr;
        }
    }

    float LevelComponent::getWaterTimeUniform() const
    {
        return _waterUniformTimer * MATH_PIX2;
    }

    bool LevelComponent::CharacterRenderer::render(SpriteAnimationComponent * animation, gameplay::SpriteBatch * spriteBatch,
                         gameplay::Matrix const & projection, SpriteAnimationComponent::Flip::Enum orientation,
                         gameplay::Vector2 const & position, gameplay::Rectangle const & viewport, float alpha,
                         gameplay::Rectangle * dstOut)
    {
        bool wasRendered = false;
        SpriteAnimationComponent::DrawTarget drawTarget = animation->getDrawTarget(gameplay::Vector2::one(), 0.0f, orientation);
        gameplay::Rectangle const bounds(position.x - ((fabs(drawTarget._scale.x / 2) * GAME_UNIT_SCALAR)),
                                                  position.y - ((fabs(drawTarget._scale.y / 2) * GAME_UNIT_SCALAR)),
                                                  fabs(drawTarget._scale.x) * GAME_UNIT_SCALAR,
                                                  fabs(drawTarget._scale.y) * GAME_UNIT_SCALAR);

        if(bounds.intersects(viewport))
        {
            gameplay::Vector2 drawPosition = position / GAME_UNIT_SCALAR;
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
            wasRendered = true;
        }

        if(dstOut)
        {
            *dstOut = std::move(bounds);
        }

        return wasRendered;
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
        font->measureText(text.c_str(), font->getSize(GAME_FONT_SIZE_LARGE_INDEX), &width, &height);
        font->drawText(text.c_str(), renderPosition.x - (width / 4), -renderPosition.y, gameplay::Vector4(1,0,0,1));
    }

    void LevelComponent::renderDebug(gameplay::Rectangle const & viewport, gameplay::Rectangle const & triggerViewport)
    {
        gameplay::Font * font = ResourceManager::getInstance().getDebugFront();
        gameplay::Rectangle const & screenDimensions = gameplay::Game::getInstance()->getViewport();

        bool const drawPositions = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_character_positions");
        bool const drawPlayerVelocity = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_player_velocity");
        bool const drawCameraTarget = gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_camera_target");
        bool const drawViewports = !gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_zoom_draw_culling");

        if(drawPlayerVelocity || drawPositions)
        {
            font->start();

            font->drawText("",0,0, gameplay::Vector4::one());

            gameplay::Matrix projection = _cameraControl->getViewProjectionMatrix();
            projection.rotateX(MATH_DEG_TO_RAD(180));
            float const spriteCameraZoomScale = (1.0f / GAME_UNIT_SCALAR) * _cameraControl->getZoom();
            float const unitToPixelScale = (1.0f / screenDimensions.height) * (screenDimensions.height * GAME_UNIT_SCALAR * spriteCameraZoomScale);
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
        projection.scale(GAME_UNIT_SCALAR, GAME_UNIT_SCALAR, 0);
        _pixelSpritebatch->setProjectionMatrix(projection);

        if(drawCameraTarget)
        {
            _pixelSpritebatch->start();

            float const dimensions = 10.0f;
            gameplay::Vector4 targetColour;
            gameplay::Game::getInstance()->getConfig()->getVector4("debug_camera_target_colour", &targetColour);
            gameplay::Rectangle targetBounds(_cameraControl->getTargetPosition().x / GAME_UNIT_SCALAR,
                                          -_cameraControl->getTargetPosition().y / GAME_UNIT_SCALAR, dimensions, dimensions);
            _pixelSpritebatch->draw(targetBounds, gameplay::Rectangle(1,1), targetColour);

            _pixelSpritebatch->finish();
        }

        if(drawViewports)
        {
            _pixelSpritebatch->setProjectionMatrix(projection);
            _pixelSpritebatch->start();
            _pixelSpritebatch->draw(getRenderDestination(viewport), gameplay::Rectangle(), gameplay::Vector4(0,0,0,0.5f));
            _pixelSpritebatch->draw(getRenderDestination(triggerViewport), gameplay::Rectangle(), gameplay::Vector4(0,1,0,0.25f));
            _pixelSpritebatch->finish();
        }

        if (gameplay::Game::getInstance()->getConfig()->getBool("debug_draw_physics"))
        {
            gameplay::Game::getInstance()->getPhysicsController()->drawDebug(getParent()->getNode()->getScene()->getActiveCamera()->getViewProjectionMatrix());
        }
    }
#endif
}
