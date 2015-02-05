#include "LevelComponent.h"

#include "Common.h"
#include "CollisionObjectComponent.h"
#include "EnemyComponent.h"
#include "Game.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include "Messages.h"
#include "PropertiesRef.h"
#include "ResourceManager.h"
#include "SpriteSheet.h"

namespace game
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

    void LevelComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
#ifndef _FINAL
        // Reload on F5 pressed so we can iterate upon it at runtime
        switch (messageType)
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
        case Messages::Type::RequestLevelReload:
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
            for(auto & collectablePair : _collectables)
            {
                Collectable & collectable = collectablePair.second;

                if(collectable._active && collectable._visible)
                {
                    float const speed = 5.0f;
                    float const height = collectable._node->getScaleY() * 0.05f;
                    float bounce = sin((gameplay::Game::getAbsoluteTime() / 1000.0f) * speed + (collectable._node->getTranslationX() + collectable._node->getTranslationY())) * height;
                    collectable._node->setTranslation(collectable._startPosition + gameplay::Vector3(0, bounce, 0));
                }
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
        GAMEOBJECTS_DELETE_MESSAGE(_loadedMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_unloadedMessage);
        GAMEOBJECTS_DELETE_MESSAGE(_preUnloadedMessage);
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
                    GAME_ASSERT(currentTileId == EMPTY_TILE || newTileId == EMPTY_TILE, 
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

            dataNamespace->rewind();
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
                    gameplay::Rectangle boumds = getObjectBounds(objectNamespace);
                    gameplay::Vector2 spawnPos(boumds.x, boumds.y);

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

            objectsNamespace->rewind();
        }
    }

    gameplay::Rectangle LevelComponent::getObjectBounds(gameplay::Properties * objectNamespace) const
    {
        gameplay::Rectangle rect;
        rect.width = objectNamespace->getInt("width") * GAME_UNIT_SCALAR;
        rect.height = objectNamespace->getInt("height") * GAME_UNIT_SCALAR;
        rect.x = objectNamespace->getInt("x") * GAME_UNIT_SCALAR;
        rect.y = ((_tileHeight * _height) - objectNamespace->getInt("y")) * GAME_UNIT_SCALAR;
        return rect;
    }

    gameplay::Node * LevelComponent::createCollisionObject(CollisionType::Enum collisionType, gameplay::Properties * collisionProperties, gameplay::Rectangle const & bounds, float rotationZ)
    {
        gameplay::Node * node = gameplay::Node::create();
        NodeCollisionInfo * info = new NodeCollisionInfo();
        info->_CollisionType = collisionType;
        node->setUserObject(info);
        node->translate(bounds.x, bounds.y, 0);
        node->rotateZ(rotationZ);
        getParent()->getNode()->addChild(node);
        node->setScale(bounds.width, bounds.height, 1.0f);

        gameplay::PhysicsCollisionObject::Type type = gameplay::PhysicsCollisionObject::GHOST_OBJECT;
        gameplay::PhysicsRigidBody::Parameters rigidBodyParams;
        gameplay::PhysicsCollisionShape::Definition shape;
        collisionProperties->getVector3("linearFactor", &rigidBodyParams.linearFactor);
        collisionProperties->getVector3("angularFactor", &rigidBodyParams.angularFactor);

        rigidBodyParams.friction = collisionProperties->getFloat("friction");
        rigidBodyParams.restitution = collisionProperties->getFloat("restitution");
        rigidBodyParams.linearDamping = collisionProperties->getFloat("linearDamping");
        rigidBodyParams.angularDamping = collisionProperties->getFloat("angularDamping");
        rigidBodyParams.mass = collisionProperties->getFloat("mass");
        rigidBodyParams.kinematic = collisionProperties->getBool("kinematic");

        if(strcmp(collisionProperties->getString("type"), "RIGID_BODY") == 0)
        {
            type = gameplay::PhysicsCollisionObject::Type::RIGID_BODY;
        }

        if(strcmp(collisionProperties->getString("shape"), "BOX") == 0)
        {
            gameplay::Vector3 extents;
            collisionProperties->getVector3("extents", &extents);
            shape = gameplay::PhysicsCollisionShape::box(extents);
        }
        else if(strcmp(collisionProperties->getString("shape"), "SPHERE") == 0)
        {
            float radius = collisionProperties->getFloat("radius");
            shape = gameplay::PhysicsCollisionShape::sphere(radius);
        }

        node->setCollisionObject(type, shape, &rigidBodyParams);
        _collisionNodes[collisionType].push_back(node);
        return node;
    }

    void setProperty(char const * id, gameplay::Vector3 const & vec, gameplay::Properties * properties)
    {
        std::array<char, 255> buffer;
        sprintf(&buffer[0], "%f, %f, %f", vec.x, vec.y, vec.z);
        properties->setString(id, &buffer[0]);
    }

    void getLineCollisionObjectParams(gameplay::Properties * lineVectorNamespace, gameplay::Rectangle & bounds, float & rotationZ, gameplay::Vector2 & direction)
    {
        gameplay::Vector2 const start(bounds.x, bounds.y);
        gameplay::Vector2 localEnd(lineVectorNamespace->getFloat("x")  * GAME_UNIT_SCALAR,
            lineVectorNamespace->getFloat("y") * GAME_UNIT_SCALAR);
        direction = localEnd;
        direction.normalize();
        rotationZ = -acos(direction.dot(gameplay::Vector2::unitX() * (direction.y > 0 ? 1.0f : -1.0f)));
        float deg = MATH_RAD_TO_DEG(rotationZ);
        gameplay::Vector2 end(start.x + localEnd.x, start.y + localEnd.y);
        gameplay::Vector2 tranlsation = start - (start + end) / 2;
        bounds.x -= tranlsation.x;
        bounds.y += tranlsation.y;
        bounds.width = start.distance(end);
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
            case CollisionType::WATER:
                collisionId = "water";
                break;
            default:
                GAME_ASSERTFAIL("Unhandled CollisionType %d", collisionType);
                break;
            }

            PropertiesRef * collisionPropertiesRef = ResourceManager::getInstance().getProperties((std::string("res/physics/level.physics#") + collisionId).c_str());
            gameplay::Properties * collisionProperties = collisionPropertiesRef->get();

            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                float rotationZ = 0.0f;
                gameplay::Rectangle bounds = getObjectBounds(objectNamespace);

                if (objectNamespace->getNamespace("polyline", true))
                {
                    if (gameplay::Properties * lineVectorNamespace = objectNamespace->getNamespace("polyline_1", true))
                    {
                        gameplay::Vector2 direction;
                        getLineCollisionObjectParams(lineVectorNamespace, bounds, rotationZ, direction);
                        static float const lineHeight = 0.05f;
                        bounds.height = lineHeight;
                    }
                }
                else
                {
                    bounds.x += bounds.width / 2;
                    bounds.y -= bounds.height / 2;
                }

                setProperty("extents", gameplay::Vector3(bounds.width, bounds.height, 1), collisionProperties);
                createCollisionObject(collisionType, collisionProperties, bounds, rotationZ);
            }

            objectsNamespace->rewind();
            SAFE_RELEASE(collisionPropertiesRef);
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

                PropertiesRef * collisionPropertiesRef = ResourceManager::getInstance().getProperties((std::string("res/physics/level.physics#") + collisionId).c_str());
                gameplay::Properties * collisionProperties = collisionPropertiesRef->get();
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
                SAFE_RELEASE(collisionPropertiesRef);
            }

            objectsNamespace->rewind();
        }
    }

    void LevelComponent::loadCollectables(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            SpriteSheet * spriteSheet = ResourceManager::getInstance().getSpriteSheet("res/spritesheets/collectables.ss");
            PropertiesRef * collisionPropertiesRef = ResourceManager::getInstance().getProperties("res/physics/level.physics#collectable");
            gameplay::Properties * collisionProperties = collisionPropertiesRef->get();
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
                        gameplay::Vector2 line(lineVectorNamespace->getFloat("x")  * GAME_UNIT_SCALAR,
                                              -lineVectorNamespace->getFloat("y") * GAME_UNIT_SCALAR);
                        float lineLength = line.length();
                        gameplay::Vector2 direction = line;
                        direction.normalize();

                        while(true)
                        {
                            Sprite & sprite = sprites[GAME_RANDOM_RANGE_INT(0, sprites.size() - 1)];
                            float const collectableWidth = sprite._src.width * GAME_UNIT_SCALAR;
                            lineLength -= collectableWidth;

                            if(lineLength > 0)
                            {
                                std::array<char, 255> radiusBuffer;
                                sprintf(&radiusBuffer[0], "%f", collectableWidth / 2);
                                collisionProperties->setString("radius", &radiusBuffer[0]);
                                gameplay::Rectangle bounds(position.x, position.y, collectableWidth, collectableWidth);
                                gameplay::Node * collectableNode = createCollisionObject(CollisionType::COLLECTABLE, collisionProperties, bounds);
                                collectableNode->addRef();
                                Collectable collectable;
                                collectable._src = sprite._src;
                                collectable._node = collectableNode;
                                collectable._startPosition = collectableNode->getTranslation();
                                collectable._active = true;
                                collectable._visible = false;
                                _collectables[collectableNode] = collectable;
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

            objectsNamespace->rewind();
            sprites.clear();
            SAFE_RELEASE(collisionPropertiesRef);
            SAFE_RELEASE(spriteSheet);
        }
    }

    void LevelComponent::loadBridges(gameplay::Properties * layerNamespace)
    {
        if (gameplay::Properties * objectsNamespace = layerNamespace->getNamespace("objects", true))
        {
            PropertiesRef * collisionPropertiesRef = ResourceManager::getInstance().getProperties("res/physics/level.physics#bridge");
            gameplay::Properties * collisionProperties = collisionPropertiesRef->get();

            while (gameplay::Properties * objectNamespace = objectsNamespace->getNextNamespace())
            {
                if (objectNamespace->getNamespace("polyline", true))
                {
                    if (gameplay::Properties * lineVectorNamespace = objectNamespace->getNamespace("polyline_1", true))
                    {
                        // Get the params for a line
                        gameplay::Rectangle bounds = getObjectBounds(objectNamespace);
                        float rotationZ = 0.0f;
                        gameplay::Vector2 bridgeDirection;
                        getLineCollisionObjectParams(lineVectorNamespace, bounds, rotationZ, bridgeDirection);

                        // Recalculate its starting position based on the size and orientation of the bridge segment(s)
                        bounds.x += (bounds.width / 2) * -bridgeDirection.x;
                        bounds.y += (bounds.width / 2) * bridgeDirection.y;
                        int const numSegments = std::ceil(bounds.width / (getTileWidth() * GAME_UNIT_SCALAR));
                        bounds.width = bounds.width / numSegments;
                        bounds.x += (bounds.width / 2) * bridgeDirection.x;
                        bounds.y += (bounds.width / 2) * -bridgeDirection.y;
                        bounds.height = (getTileHeight() * GAME_UNIT_SCALAR) * 0.25f;
                        setProperty("extents", gameplay::Vector3(bounds.width, bounds.height, 0.0f), collisionProperties);

                        // Create collision nodes for them
                        std::vector<gameplay::Node *> segmentNodes;
                        for (int i = 0; i < numSegments; ++i)
                        {
                            segmentNodes.push_back(createCollisionObject(CollisionType::BRIDGE, collisionProperties, bounds, rotationZ));
                            bounds.x += bridgeDirection.x * bounds.width;
                            bounds.y -= bridgeDirection.y * bounds.width;
                        }

                        // Link them to each other and the end pieces with the world
                        for (int segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
                        {
                            gameplay::Node * segmentNode = segmentNodes[segmentIndex];
                            gameplay::PhysicsRigidBody * segmentRigidBody = static_cast<gameplay::PhysicsRigidBody*>(segmentNode->getCollisionObject());
                            gameplay::Vector3 const hingeOffset((bounds.width / 2) * (1.0f / segmentNode->getScaleX()) * (bridgeDirection.y >= 0 ? 1.0f : -1.0f), 0.0f, 0.0f);
                            gameplay::PhysicsController * physicsController = gameplay::Game::getInstance()->getPhysicsController();

                            bool const isFirstSegment = segmentIndex == 0;
                            if (isFirstSegment)
                            {
                                physicsController->createHingeConstraint(segmentRigidBody, gameplay::Quaternion(), -hingeOffset);
                            }

                            bool const isEndSegment = segmentIndex == numSegments - 1;
                            if (!isEndSegment)
                            {
                                gameplay::PhysicsRigidBody * nextSegmentRigidBody = static_cast<gameplay::PhysicsRigidBody*>(segmentNodes[segmentIndex + 1]->getCollisionObject());
                                physicsController->createHingeConstraint(segmentRigidBody, gameplay::Quaternion(), hingeOffset, nextSegmentRigidBody, gameplay::Quaternion(), -hingeOffset);
                            }
                            else
                            {
                                physicsController->createHingeConstraint(segmentRigidBody, gameplay::Quaternion(), hingeOffset);
                            }
                        }
                    }
                }
            }

            objectsNamespace->rewind();

            SAFE_RELEASE(collisionPropertiesRef);
        }
    }

    void LevelComponent::load()
    {
        PropertiesRef * rootRef = ResourceManager::getInstance().getProperties(_level.c_str());
        gameplay::Properties * root = rootRef->get();

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
                    else if (layerName == "collision_water")
                    {
                        collisionType = CollisionType::WATER;
                    }
                    else if (layerName == "collision_bridge")
                    {
                        collisionType = CollisionType::BRIDGE;
                    }

                    if (collisionType != CollisionType::BRIDGE)
                    {
                        loadStaticCollision(layerNamespace, collisionType);
                    }
                    else
                    {
                        loadBridges(layerNamespace);
                    }
                }
                else if (layerName == "interactive_props")
                {
#ifndef _FINAL
                    if (gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_interactables_spawn"))
#endif
                        loadDynamicCollision(layerNamespace);
                }
                else if(layerName == "collectables")
                {
#ifndef _FINAL
                    if (gameplay::Game::getInstance()->getConfig()->getBool("debug_enable_collectables_spawn"))
#endif
                        loadCollectables(layerNamespace);
                }
            }

            layersNamespace->rewind();
        }

        placeEnemies();
        root->rewind();
        SAFE_RELEASE(rootRef);
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
                    gameplay::Vector3 const collisionNodePosition = collisionNode->getTranslation();

                    if (collisionNodePosition.y <= enemyPosition.y)
                    {
                        float const distance = collisionNode->getTranslation().distanceSquared(enemyPosition);

                        if (distance < nearestDistance)
                        {
                            nearestCollisionNode = collisionNode;
                            nearestDistance = distance;
                        }
                    }
                });

                if(nearestCollisionNode)
                {
                    if (enemyComponent->isSnappedToCollisionY())
                    {
                        enemyComponent->getTriggerNode()->setTranslationY(nearestCollisionNode->getTranslationY() +
                            nearestCollisionNode->getScaleY() / 2 +
                            enemyComponent->getTriggerNode()->getScaleY() / 2);
                    }

                    enemyComponent->setHorizontalConstraints(nearestCollisionNode->getTranslationX() - (nearestCollisionNode->getScaleX() / 2) - (enemyComponent->getTriggerNode()->getScaleX() / 2),
                                                    nearestCollisionNode->getTranslationX() + (nearestCollisionNode->getScaleX() / 2) + (enemyComponent->getTriggerNode()->getScaleX() / 2));
                }
                else
                {
                    GAME_ASSERTFAIL("Unable to place %s", enemyComponent->getId().c_str());
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
                NodeCollisionInfo * info = NodeCollisionInfo::getNodeCollisionInfo(node);
                node->setUserObject(nullptr);
                SAFE_RELEASE(info);
                getParent()->getNode()->removeChild(node);
                SAFE_RELEASE(node);
            }
        }

        for(auto & collectablePair : _collectables)
        {
            getParent()->getNode()->removeChild(collectablePair.second._node);
            SAFE_RELEASE(collectablePair.second._node);
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
        GAME_ASSERT(gameplay::FileSystem::fileExists(_level.c_str()), "Level '%s' not found", _level.c_str());
    }

    void LevelComponent::consumeCollectable(gameplay::Node * collectableNode)
    {
        auto itr = _collectables.find(collectableNode);

        if(itr != _collectables.end())
        {
            Collectable & collectable = itr->second;
            collectable._active = false;
        }
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

    void LevelComponent::getCollectables(std::vector<Collectable *> & collectablesOut)
    {
        for (auto & collectablePair : _collectables)
        {
            collectablesOut.push_back(&collectablePair.second);
        }
    }
}
