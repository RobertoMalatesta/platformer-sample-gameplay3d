#include "LevelComponent.h"

#include "Common.h"
#include "CollisionObjectComponent.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "Messages.h"
#include "MessagesInput.h"
#include "MessagesLevel.h"

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
                        if (gameplay::PhysicsCharacter * character = collisionComponent->getNode()->getCollisionObject()->asCharacter())
                        {
                            if (character->isPhysicsEnabled())
                            {
                                collisionComponent->getNode()->setTranslation(spawnPos.x, spawnPos.y, 0);
                            }
                        }
                    }

                    _children.push_back(gameObject);
                }
            }
        }
    }

    void LevelComponent::loadCollision(gameplay::Properties * layerNamespace, CollisionType::Enum terrainType)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            std::string collisionId;

            switch (terrainType)
            {
            case CollisionType::COLLISION:
                collisionId = "world_collision";
                break;
            case CollisionType::LADDER:
                collisionId = "ladder";
                break;
            case CollisionType::RESET:
                collisionId = "reset";
                break;
            default:
                PLATFORMER_ASSERTFAIL("Unhandled CollisionType %d", terrainType);
                break;
            }

            gameplay::Properties * collisionProperties = createProperties((std::string("res/physics/level.physics#") + collisionId).c_str());

            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                float rotationZ = 0.0f;
                float width = objectNamespace->getInt("width") * PLATFORMER_UNIT_SCALAR;
                float height = objectNamespace->getInt("height") * PLATFORMER_UNIT_SCALAR;
                float x = objectNamespace->getInt("x") * PLATFORMER_UNIT_SCALAR;
                float y = -objectNamespace->getInt("y") * PLATFORMER_UNIT_SCALAR;

                if (gameplay::Properties * lineNamespace = objectNamespace->getNamespace("polyline", true))
                {
                    if (gameplay::Properties * lineVectorNamespace = objectNamespace->getNamespace("polyline_1", true))
                    {
                        gameplay::Vector2 const start(x, y);
                        gameplay::Vector2 end(start.x + (lineVectorNamespace->getFloat("x")  * PLATFORMER_UNIT_SCALAR), 
                            start.y + (lineVectorNamespace->getFloat("y") * PLATFORMER_UNIT_SCALAR));
                        gameplay::Vector2 direction = start - end;
                        direction.normalize();
                        rotationZ = -acos(direction.dot(gameplay::Vector2::unitX()));
                        gameplay::Vector2 tranlsation = start - (start + end) / 2;
                        x -= tranlsation.x;
                        y += tranlsation.y;
                        static const float lineHeight = 0.05f;
                        width = start.distance(end);
                        height = lineHeight;
                    }
                }
                else
                {
                    x += width / 2;
                    y -= height / 2;
                }

                std::array<char, 255> extentsBuffer;
                sprintf(&extentsBuffer[0], "%f, %f, 1", width, height);
                collisionProperties->setString("extents", &extentsBuffer[0]);

                gameplay::Node * node = gameplay::Node::create();
                TerrainInfo * info = new TerrainInfo();
                info->_CollisionType = terrainType;
                node->setUserPointer(info);
                node->translate(x, y, 0);
                node->rotateZ(rotationZ);
                getParent()->getNode()->addChild(node);
                node->setCollisionObject(collisionProperties);
                _collisionNodes[terrainType].push_back(node);
            }

            SAFE_DELETE(collisionProperties);
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
                    CollisionType::Enum terrainType = CollisionType::COLLISION;

                    if (layerName == "collision_ladder")
                    {
                        terrainType = CollisionType::LADDER;
                    }
                    else if (layerName == "collision_hand_of_god")
                    {
                        terrainType = CollisionType::RESET;
                    }

                    loadCollision(layerNamespace, terrainType);
                }
            }
        }

        SAFE_DELETE(root);
    }

    void LevelComponent::unload()
    {
        for (auto & listPair : _collisionNodes)
        {
            for (gameplay::Node* node : listPair.second)
            {
                TerrainInfo * info = TerrainInfo::getTerrainInfo(node);
                node->setUserPointer(nullptr);
                delete info;
                getParent()->getNode()->removeChild(node);
                SAFE_RELEASE(node);
            }
        }

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

    void LevelComponent::forEachCachedNode(CollisionType::Enum terrainType, std::function<void(gameplay::Node *)> func)
    {
        for (gameplay::Node * node : _collisionNodes[terrainType])
        {
            func(node);
        }
    }
}
