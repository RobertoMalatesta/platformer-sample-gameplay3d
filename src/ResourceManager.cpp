#include "ResourceManager.h"

#include "Common.h"
#include "FileSystem.h"
#include "Font.h"
#include "Game.h"
#include "PropertiesRef.h"
#include "SpriteBatch.h"
#include "SpriteSheet.h"
#include "Texture.h"

namespace game
{
    static char const * PIXEL_TEXTURE_PATH = "pixel";

    ResourceManager::ResourceManager()
        : _debugFont(nullptr)
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

    void ResourceManager::initializeForBoot()
    {
        PERF_SCOPE("ResourceManager::initializeForBoot");
#ifndef _FINAL
        {
            std::string const fontPath = gameplay::Game::getInstance()->getConfig()->getString("debug_font");
            PERF_SCOPE(fontPath);
            _debugFont = gameplay::Font::create(fontPath.c_str());
            _debugFont->addRef();
        }
#endif
        {
            PERF_SCOPE(PIXEL_TEXTURE_PATH);
            std::array<unsigned char, 4> rgba;
            rgba.fill(std::numeric_limits<unsigned char>::max());
            cacheTexture(PIXEL_TEXTURE_PATH, gameplay::Texture::create(gameplay::Texture::Format::RGBA, 1, 1, &rgba.front()));
        }

        if(gameplay::Properties * bootNs = gameplay::Game::getInstance()->getConfig()->getNamespace("boot", true))
        {
            if(gameplay::Properties * bootTexturesNs = bootNs->getNamespace("textures", true))
            {
                while(char const * texturePath = bootTexturesNs->getNextProperty())
                {
                    cacheTexture(texturePath);
                }
            }

            if(gameplay::Properties * bootSpritesheetsNs = bootNs->getNamespace("spritesheets", true))
            {
                while(char const * spritesheetPath = bootSpritesheetsNs->getNextProperty())
                {
                    cacheProperties(spritesheetPath);
                    cacheSpriteSheet(spritesheetPath);
                }
            }
        }
    }

    void ResourceManager::initialize()
    {
        PERF_SCOPE("ResourceManager::initialize");

        std::vector<std::string> fileList;

        std::string const textureDirectory = gameplay::FileSystem::resolvePath("@res/textures");
        gameplay::FileSystem::listFiles(textureDirectory.c_str(), fileList);

        for (std::string & fileName : fileList)
        {
            STALL_SCOPE();
            cacheTexture(textureDirectory + "/" + fileName);
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
                    PropertiesRef * propertiesRef = getProperties(propertyPath);
                    bool const usesTopLevelNamespaceUrls = propertyDirNamespace->getBool();

                    if (!propertiesRef)
                    {
                        STALL_SCOPE();
                        PERF_SCOPE(propertyPath);
                        propertiesRef = new PropertiesRef(gameplay::Properties::create(propertyPath.c_str()));

                        if (!usesTopLevelNamespaceUrls)
                        {
                            propertiesRef->addRef();
                        }
                    }
                    else
                    {
                        propertiesRef->release();
                    }

                    if(usesTopLevelNamespaceUrls)
                    {
                        gameplay::Properties * properties = propertiesRef->get();

                        while(gameplay::Properties * topLevelChildNS = properties->getNextNamespace())
                        {
                            STALL_SCOPE();
                            cacheProperties(propertyPath + std::string("#") + topLevelChildNS->getId());
                        }

                        SAFE_RELEASE(propertiesRef);
                    }
                    else
                    {
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
            STALL_SCOPE();
            cacheSpriteSheet(spriteSheetDirectory + "/" + fileName);
        }
    }

    void ResourceManager::cacheTexture(std::string const & texturePath)
    {
        if(_cachedTextures.find(texturePath) == _cachedTextures.end())
        {
            PERF_SCOPE(texturePath);
            gameplay::Texture * texture = gameplay::Texture::create(texturePath.c_str());
            texture->addRef();
            _cachedTextures[texturePath] = texture;
        }
    }

    void ResourceManager::cacheTexture(std::string const & texturePath, gameplay::Texture * texture)
    {
        texture->addRef();
        _cachedTextures[texturePath] = texture;
    }

    void ResourceManager::cacheSpriteSheet(std::string const & spritesheetPath)
    {
        if(_cachedSpriteSheets.find(spritesheetPath) == _cachedSpriteSheets.end())
        {
            PERF_SCOPE(spritesheetPath);
            SpriteSheet * spriteSheet = new SpriteSheet();
            spriteSheet->initialize(spritesheetPath);
            spriteSheet->addRef();
            _cachedSpriteSheets[spritesheetPath] = spriteSheet;
        }
    }

    void ResourceManager::cacheProperties(std::string const & propertiesPath)
    {
        if(_cachedProperties.find(propertiesPath) == _cachedProperties.end())
        {
            PERF_SCOPE(propertiesPath);
            PropertiesRef * childPropertiesRef = new PropertiesRef(gameplay::Properties::create(propertiesPath.c_str()));
            childPropertiesRef->addRef();
            _cachedProperties[propertiesPath] = childPropertiesRef;
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

        for(auto pair : _cachedTextures)
        {
            releaseCacheRefs(pair.second);
        }

        _cachedProperties.clear();
        _cachedSpriteSheets.clear();
        _cachedTextures.clear();

#ifndef _FINAL
        releaseCacheRefs(_debugFont);
#endif
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
        return gameplay::SpriteBatch::create(_cachedTextures[PIXEL_TEXTURE_PATH]);
    }

#ifndef _FINAL
    gameplay::Font * ResourceManager::getDebugFront() const
    {
        return _debugFont;
    }
#endif
}
