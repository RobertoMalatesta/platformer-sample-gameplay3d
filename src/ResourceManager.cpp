#include "ResourceManager.h"

#include "Common.h"
#include "FileSystem.h"
#include "Game.h"
#include "PropertiesRef.h"
#include "SpriteBatch.h"
#include "SpriteSheet.h"
#include "Texture.h"

namespace game
{
    ResourceManager::ResourceManager()
    {
    }

    ResourceManager::~ResourceManager()
    {
    }

    ResourceManager::ResourceManager(ResourceManager const &)
    {
    }

    ResourceManager & ResourceManager::getInstance()
    {
        static ResourceManager instance;
        return instance;
    }

#ifndef _FINAL
    struct LoadScope
    {
        LoadScope(std::string const & url)
        {
            _start = gameplay::Game::getAbsoluteTime();
            _url = url;
        }

        ~LoadScope()
        {
            GAME_LOG("%.2fs loading %s", (gameplay::Game::getAbsoluteTime() - _start) * 0.001, _url.c_str());
        }

        double _start;
        std::string _url;
    };

    #define LOAD_SCOPE(url) LoadScope scope(url);
#else
    #define LOAD_SCOPE(url)
#endif

    void ResourceManager::initialize()
    {
        std::vector<std::string> fileList;

        std::string const textureDirectory = gameplay::FileSystem::resolvePath("@res/textures");
        gameplay::FileSystem::listFiles(textureDirectory.c_str(), fileList);

        for (std::string & fileName : fileList)
        {
            std::string const texturePath = textureDirectory + "/" + fileName;
            LOAD_SCOPE(texturePath)
            gameplay::Texture * texture = gameplay::Texture::create(texturePath.c_str());
            texture->addRef();
            _cachedTextures.push_back(texture);
        }

        if(gameplay::Properties * propertyDirNamespace = gameplay::Game::getInstance()->getConfig()->getNamespace("properties_directories", true))
        {
            while(char const * dir = propertyDirNamespace->getNextProperty())
            {
                fileList.clear();
                gameplay::FileSystem::listFiles(dir, fileList);

                for(std::string & propertyUrl : fileList)
                {
                    std::string const propertyPath = std::string(dir) + std::string("/") + propertyUrl;
                    PropertiesRef * propertiesRef = nullptr;

                    {
                        LOAD_SCOPE(propertyPath)
                        propertiesRef = new PropertiesRef(gameplay::Properties::create(propertyPath.c_str()));
                    }

                    bool const usesTopLevelNamespaceUrls = propertyDirNamespace->getBool();

                    if(usesTopLevelNamespaceUrls)
                    {
                        gameplay::Properties * properties = propertiesRef->get();

                        while(gameplay::Properties * topLevelChildNS = properties->getNextNamespace())
                        {
                            std::string const childPropertiesPath = std::string(propertyPath + std::string("#") + topLevelChildNS->getId()).c_str();
                            PropertiesRef * childPropertiesRef = nullptr;

                            {
                                LOAD_SCOPE(childPropertiesPath)
                                childPropertiesRef = new PropertiesRef(gameplay::Properties::create(childPropertiesPath.c_str()));
                            }

                            childPropertiesRef->addRef();
                            _cachedProperties[childPropertiesPath] = childPropertiesRef;
                        }

                        SAFE_RELEASE(propertiesRef);
                    }
                    else
                    {
                        propertiesRef->addRef();
                        _cachedProperties[propertyPath] = propertiesRef;
                    }
                }
            }
        }

        fileList.clear();
        std::string const spriteSheetDirectory = "res/spritesheets";
        gameplay::FileSystem::listFiles(spriteSheetDirectory.c_str(), fileList);

        for (std::string & fileName : fileList)
        {
            std::string const spriteSheetPath = spriteSheetDirectory + "/" + fileName;
            LOAD_SCOPE(spriteSheetPath)
            SpriteSheet * spriteSheet = new SpriteSheet();
            spriteSheet->initialize(spriteSheetPath);
            spriteSheet->addRef();
            _cachedSpriteSheets[spriteSheetPath] = spriteSheet;
        }
    }

    void releaseCacheRefs(gameplay::Ref * ref)
    {
        if (ref)
        {
            for(int i = 0; i < 2; ++i)
            {
                ref->release();
            }
        }
    }

    void ResourceManager::finalize()
    {
        for(auto pair : _cachedProperties)
        {
            releaseCacheRefs(pair.second);
        }

        for(auto pair : _cachedSpriteSheets)
        {
            releaseCacheRefs(pair.second);
        }

        for (gameplay::Ref * texture : _cachedTextures)
        {
            releaseCacheRefs(texture);
        }

        _cachedProperties.clear();
        _cachedSpriteSheets.clear();
        _cachedTextures.clear();
    }

    PropertiesRef * ResourceManager::getProperties(std::string const & url)
    {
        auto itr = _cachedProperties.find(url);

        if(itr != _cachedProperties.end())
        {
            itr->second->addRef();
            return itr->second;
        }

        return nullptr;
    }

    SpriteSheet * ResourceManager::getSpriteSheet(std::string const & url)
    {
        auto itr = _cachedSpriteSheets.find(url);

        if(itr != _cachedSpriteSheets.end())
        {
            itr->second->addRef();
            return itr->second;
        }

        return nullptr;
    }

    gameplay::SpriteBatch * ResourceManager::createSinglePixelSpritebatch()
    {
        std::array<unsigned char, 4> rgba;
        rgba.fill(std::numeric_limits<unsigned char>::max());
        gameplay::Texture * texture = gameplay::Texture::create(gameplay::Texture::Format::RGBA, 1, 1, &rgba.front());
        gameplay::SpriteBatch * spriteBatch = gameplay::SpriteBatch::create(texture);
        texture->release();
        return spriteBatch;
    }
}
