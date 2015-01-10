#include "LevelComponent.h"

#include "Common.h"
#include "CollisionObjectComponent.h"
#include "EnemyComponent.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "Messages.h"
#include "MessagesInput.h"
#include "MessagesLevel.h"
#include "SpriteSheet.h"

namespace platformer
{
    LevelComponent::LevelComponent()
        : _loadedMessage(nullptr)
        , _unloadedMessage(nullptr)
        , _preUnloadedMessage(nullptr)
        , _loadBroadcasted(false)
    {
    }

    LevelComponent::~LevelComponent()
    {
    }

    void LevelComponent::onMessageReceived(gameplay::AIMessage * message)
    {
#ifndef _FINAL
        // Reload on F5 pressed so we can iterate upon it at runtime
        switch (message->getId())
        {
        case Messages::Type::Key:
            {
                KeyMessage keyMessage(message);
                if(keyMessage._event == gameplay::Keyboard::KeyEvent::KEY_PRESS && keyMessage._key == gameplay::Keyboard::Key::KEY_F5)
                {
                    reload();
                }
            }
            break;
        case Messages::Type::RequestLevelReloadMessage:
            reload();
            break;
        }
#endif
    }

    void LevelComponent::reload()
    {
        _loadBroadcasted = false;
    }

    void LevelComponent::update(float)
    {
        if(!_loadBroadcasted)
        {
            getRootParent()->broadcastMessage(_preUnloadedMessage);
            unload();
            getRootParent()->broadcastMessage(_unloadedMessage);
            load();
            getRootParent()->broadcastMessage(_loadedMessage);
            _loadBroadcasted = true;
        }

        if(!_collectables.empty())
        {
            for(Collectable & collectable : _collectables)
            {
                float const speed = 2.75f;
                float const height = collectable._node->getScaleY() * 0.15f;
                float bounce = sin((gameplay::Game::getAbsoluteTime() / 1000.0f) * speed + (collectable._node->getTranslationX())) * height;
                collectable._node->setTranslation(collectable._startPosition + gameplay::Vector3(0,bounce,0));
            }
        }
    }

    void LevelComponent::initialize()
    {
        _loadedMessage = LevelLoadedMessage::create();
        _unloadedMessage = LevelUnloadedMessage::create();
        _preUnloadedMessage = PreLevelUnloadedMessage::create();
    }

    void LevelComponent::finalize()
    {
        unload();
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_loadedMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_unloadedMessage);
        PLATFORMER_SAFE_DELETE_AI_MESSAGE(_preUnloadedMessage);
    }

    void LevelComponent::loadTerrain(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * dataNamespace = layerNamespace->getNamespace("data", true))
        {
            int x = 0;
            int y = 0;

            while (dataNamespace->getNextProperty())
            {
                int const currentTileId = _grid[y][x]._tileId;

                if (currentTileId == EMPTY_TILE)
                {
                    int const newTileId = dataNamespace->getInt();
                    PLATFORMER_ASSERT(currentTileId == EMPTY_TILE || newTileId == EMPTY_TILE, 
                        "Multi layered tile rendering not isn't supported [%d][%d]", x, y);
                    _grid[y][x]._tileId = newTileId;
                }

                ++x;

                if (x == _width)
                {
                    x = 0;
                    ++y;

                    if (y == _height)
                    {
                        x = 0;
                        y = 0;
                        break;
                    }
                }
            }
        }
    }

    void LevelComponent::loadCharacters(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                char const * gameObjectTypeName = objectNamespace->getString("name");
                bool const isPlayer = strcmp(gameObjectTypeName, "player") == 0;

#ifndef _FINAL
                if (gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_enemy_spawn") || isPlayer)
#endif
                {
                    gameobjects::GameObject * gameObject = gameobjects::GameObjectController::getInstance().createGameObject(gameObjectTypeName, getParent());
                    gameplay::Vector2 spawnPos(objectNamespace->getInt("x"), -objectNamespace->getInt("y"));
                    spawnPos *= PLATFORMER_UNIT_SCALAR;

                    if (isPlayer)
                    {
                        _playerSpawnPosition = spawnPos;
                    }

                    std::vector<CollisionObjectComponent*> collisionComponents;
                    gameObject->getComponents(collisionComponents);

                    for (CollisionObjectComponent * collisionComponent : collisionComponents)
                    {
                        collisionComponent->getNode()->setTranslation(spawnPos.x, spawnPos.y, 0);
                    }

                    _children.push_back(gameObject);
                }
            }
        }
    }

    gameplay::Rectangle LevelComponent::getObjectBounds(gameplay::Properties * objectNamespace) const
    {
        gameplay::Rectangle rect;
        rect.width = objectNamespace->getInt("width") * PLATFORMER_UNIT_SCALAR;
        rect.height = objectNamespace->getInt("height") * PLATFORMER_UNIT_SCALAR;
        rect.x = objectNamespace->getInt("x") * PLATFORMER_UNIT_SCALAR;
        rect.y = -objectNamespace->getInt("y") * PLATFORMER_UNIT_SCALAR;
        return rect;
    }

    void LevelComponent::createCollisionObject(CollisionType::Enum collisionType, gameplay::Properties * collisionProperties, gameplay::Rectangle const & bounds, float rotationZ)
    {
        gameplay::Node * node = gameplay::Node::create();
        TerrainInfo * info = new TerrainInfo();
        info->_CollisionType = collisionType;
        node->setDrawable(info);
        node->translate(bounds.x, bounds.y, 0);
        node->rotateZ(rotationZ);
        getParent()->getNode()->addChild(node);
        node->setScaleX(bounds.width);
        node->setScaleY(bounds.height);
        node->setCollisionObject(collisionProperties);
        _collisionNodes[collisionType].push_back(node);
    }

    void LevelComponent::loadStaticCollision(gameplay::Properties * layerNamespace, CollisionType::Enum collisionType)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            std::string collisionId;

            switch (collisionType)
            {
            case CollisionType::COLLISION_STATIC:
                collisionId = "world_collision";
                break;
            case CollisionType::LADDER:
                collisionId = "ladder";
                break;
            case CollisionType::RESET:
                collisionId = "reset";
                break;
            default:
                PLATFORMER_ASSERTFAIL("Unhandled CollisionType %d", collisionType);
                break;
            }

            gameplay::Properties * collisionProperties = createProperties((std::string("res/physics/level.physics#") + collisionId).c_str());

            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                float rotationZ = 0.0f;
                gameplay::Rectangle bounds = getObjectBounds(objectNamespace);

                if (objectNamespace->getNamespace("polyline", true))
                {
                    if (gameplay::Properties * lineVectorNamespace = objectNamespace->getNamespace("polyline_1", true))
                    {
                        gameplay::Vector2 const start(bounds.x, bounds.y);
                        gameplay::Vector2 end(start.x + (lineVectorNamespace->getFloat("x")  * PLATFORMER_UNIT_SCALAR), 
                            start.y + (lineVectorNamespace->getFloat("y") * PLATFORMER_UNIT_SCALAR));
                        gameplay::Vector2 direction = start - end;
                        direction.normalize();
                        rotationZ = -acos(direction.dot(gameplay::Vector2::unitX()));
                        gameplay::Vector2 tranlsation = start - (start + end) / 2;
                        bounds.x -= tranlsation.x;
                        bounds.y += tranlsation.y;
                        static const float lineHeight = 0.05f;
                        bounds.width = start.distance(end);
                        bounds.height = lineHeight;
                    }
                }
                else
                {
                    bounds.x += bounds.width / 2;
                    bounds.y -= bounds.height / 2;
                }

                std::array<char, 255> extentsBuffer;
                sprintf(&extentsBuffer[0], "%f, %f, 1", bounds.width, bounds.height);
                collisionProperties->setString("extents", &extentsBuffer[0]);
                createCollisionObject(collisionType, collisionProperties, bounds, rotationZ);
            }

            SAFE_DELETE(collisionProperties);
        }
    }

    void LevelComponent::loadDynamicCollision(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                bool const isBoulder = objectNamespace->exists("ellipse");
                std::string collisionId = isBoulder ? "boulder" : "crate";

                gameplay::Properties * collisionProperties = createProperties((std::string("res/physics/level.physics#") + collisionId).c_str());
                gameplay::Rectangle bounds = getObjectBounds(objectNamespace);
                std::array<char, 255> dimensionsBuffer;
                std::string dimensionsId = "extents";
                bounds.x += bounds.width / 2;
                bounds.y -= bounds.height / 2;

                if (isBoulder)
                {
                    dimensionsId = "radius";
                    sprintf(&dimensionsBuffer[0], "%f", bounds.height / 2);
                }
                else
                {
                    sprintf(&dimensionsBuffer[0], "%f, %f, 1", bounds.width, bounds.height);
                }

                collisionProperties->setString(dimensionsId.c_str(), &dimensionsBuffer[0]);
                createCollisionObject(CollisionType::COLLISION_DYNAMIC, collisionProperties, bounds);
                SAFE_DELETE(collisionProperties);
            }
        }
    }

    void LevelComponent::loadCollectables(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            SpriteSheet * spriteSheet = SpriteSheet::create("res/spritesheets/collectables.ss");
            gameplay::Properties * collisionProperties = createProperties("res/physics/level.physics#collectable");
            std::vector<Sprite> sprites;

            spriteSheet->forEachSprite([&sprites](Sprite const & sprite)
            {
                sprites.push_back(sprite);
            });

            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                gameplay::Rectangle const dst = getObjectBounds(objectNamespace);
                gameplay::Vector2 position(dst.x, dst.y);

                if (objectNamespace->getNamespace("polyline", true))
                {
                    if (gameplay::Properties * lineVectorNamespace = objectNamespace->getNamespace("polyline_1", true))
                    {
                        gameplay::Vector2 line(lineVectorNamespace->getFloat("x")  * PLATFORMER_UNIT_SCALAR,
                                              -lineVectorNamespace->getFloat("y") * PLATFORMER_UNIT_SCALAR);
                        float lineLength = line.length();
                        gameplay::Vector2 direction = line;
                        direction.normalize();

                        while(true)
                        {
                            Sprite & sprite = sprites[PLATFORMER_RANDOM_RANGE_INT(0, sprites.size() - 1)];
                            float const collectableWidth = sprite._src.width * PLATFORMER_UNIT_SCALAR;
                            lineLength -= collectableWidth;

                            if(lineLength > 0)
                            {
                                std::array<char, 255> radiusBuffer;
                                sprintf(&radiusBuffer[0], "%f", collectableWidth / 2);
                                collisionProperties->setString("radius", &radiusBuffer[0]);
                                gameplay::Node * collectableNode =
                                        gameplay::Node::create((std::string("collectable_") + std::to_string(_collectables.size())).c_str());
                                collectableNode->setTranslation(position.x, position.y, 0);
                                collectableNode->setScale(collectableWidth);
                                getParent()->getNode()->addChild(collectableNode);
                                collectableNode->setCollisionObject(collisionProperties);
                                Collectable collectable;
                                collectable._src = sprite._src;
                                collectable._node = collectableNode;
                                collectable._startPosition = collectableNode->getTranslation();
                                _collectables.push_back(collectable);
                                float const padding = 1.25f;
                                position += direction * (collectableWidth * padding);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }

            sprites.clear();
            SAFE_DELETE(collisionProperties);
            SAFE_RELEASE(spriteSheet);
        }
    }

    void LevelComponent::load()
    {
        gameplay::Properties * root = createProperties(_level.c_str());

        if (gameplay::Properties * propertiesNamespace = root->getNamespace("properties", true, false))
        {
            _texturePath = propertiesNamespace->getString("texture");
        }

        _width = root->getInt("width");
        _height = root->getInt("height");
        _tileWidth = root->getInt("tilewidth");
        _tileHeight = root->getInt("tileheight");
        _grid.resize(_height);
        
        for (std::vector<Tile> & horizontalTiles : _grid)
        {
            horizontalTiles.resize(_width);
        }

        if (gameplay::Properties * layersNamespace = root->getNamespace("layers", true))
        {
            while (gameplay::Properties * layerNamespace = layersNamespace->getNextNamespace())
            {
                std::string const layerName = layerNamespace->getString("name");

                if (layerName == "terrain" || layerName == "props")
                {
                    loadTerrain(layerNamespace);
                }
                else if (layerName == "characters")
                {
                    loadCharacters(layerNamespace);
                }
                else if (layerName.find("collision") != std::string::npos)
                {
                    CollisionType::Enum collisionType = CollisionType::COLLISION_STATIC;

                    if (layerName == "collision_ladder")
                    {
                        collisionType = CollisionType::LADDER;
                    }
                    else if (layerName == "collision_hand_of_god")
                    {
                        collisionType = CollisionType::RESET;
                    }

                    loadStaticCollision(layerNamespace, collisionType);
                }
                else if (layerName == "interactive_props")
                {
                    loadDynamicCollision(layerNamespace);
                }
                else if(layerName == "collectables")
                {
                    loadCollectables(layerNamespace);
                }
            }
        }

        placeEnemies();

        SAFE_DELETE(root);
    }

    void LevelComponent::placeEnemies()
    {
        for(gameobjects::GameObject * gameObject : _children)
        {
            if(EnemyComponent * enemyComponent = gameObject->getComponent<EnemyComponent>())
            {
                gameplay::Node * nearestCollisionNode = nullptr;
                float nearestDistance = std::numeric_limits<float>::max();
                gameplay::Vector3 const & enemyPosition = enemyComponent->getTriggerNode()->getTranslation();

                forEachCachedNode(CollisionType::COLLISION_STATIC, [&nearestCollisionNode, &nearestDistance, &enemyPosition](gameplay::Node * collisionNode)
                {
                    float const distance = collisionNode->getTranslation().distanceSquared(enemyPosition);

                    if(distance < nearestDistance)
                    {
                        nearestCollisionNode = collisionNode;
                        nearestDistance = distance;
                    }
                });

                if(nearestCollisionNode)
                {
                    enemyComponent->getTriggerNode()->setTranslationY(nearestCollisionNode->getTranslationY() +
                                                                      nearestCollisionNode->getScaleY() / 2 +
                                                                      enemyComponent->getTriggerNode()->getScaleY() / 2);
                    enemyComponent->setHorizontalConstraints(nearestCollisionNode->getTranslationX() - (nearestCollisionNode->getScaleX() / 2) - (enemyComponent->getTriggerNode()->getScaleX() / 2),
                                                    nearestCollisionNode->getTranslationX() + (nearestCollisionNode->getScaleX() / 2) + (enemyComponent->getTriggerNode()->getScaleX() / 2));
                }
            }
        }
    }

    void LevelComponent::unload()
    {
        for (auto & listPair : _collisionNodes)
        {
            for (gameplay::Node* node : listPair.second)
            {
                TerrainInfo * info = TerrainInfo::getTerrainInfo(node);
                node->setDrawable(nullptr);
                delete info;
                getParent()->getNode()->removeChild(node);
                SAFE_RELEASE(node);
            }
        }

        for(Collectable & collectable : _collectables)
        {
            getParent()->getNode()->removeChild(collectable._node);
            SAFE_RELEASE(collectable._node);
        }

        _collectables.clear();
        _collisionNodes.clear();

        _grid.clear();

        for(auto childItr = _children.begin(); childItr != _children.end(); ++childItr)
        {
            gameobjects::GameObjectController::getInstance().destroyGameObject(*childItr);
        }

        _children.clear();
    }

    void LevelComponent::readProperties(gameplay::Properties & properties)
    {
        _level = properties.getString("level", _level.c_str());
        PLATFORMER_ASSERT(gameplay::FileSystem::fileExists(_level.c_str()), "Level '%s' not found", _level.c_str());
    }

    std::string const & LevelComponent::getTexturePath() const
    {
        return _texturePath;
    }

    int LevelComponent::getTileWidth() const
    {
        return _tileWidth;
    }

    int LevelComponent::getTileHeight() const
    {
        return _tileHeight;
    }

    int LevelComponent::getTile(int x, int y) const
    {
        return _grid[y][x]._tileId;
    }

    int LevelComponent::getWidth() const
    {
        return _width;
    }

    int LevelComponent::getHeight() const
    {
        return _height;
    }

    gameplay::Vector2 const & LevelComponent::getPlayerSpawnPosition() const
    {
        return _playerSpawnPosition;
    }

    void LevelComponent::forEachCachedNode(CollisionType::Enum collisionType, std::function<void(gameplay::Node *)> func)
    {
        for (gameplay::Node * node : _collisionNodes[collisionType])
        {
            func(node);
        }
    }

    void LevelComponent::forEachActiveCollectable(std::function<void(Collectable const &)> func)
    {
        for (Collectable & collectable : _collectables)
        {
            func(collectable);
        }
    }
}
