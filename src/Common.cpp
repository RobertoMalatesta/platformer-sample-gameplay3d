#include "Common.h"

#include "Font.h"
#include "ScreenDisplayer.h"
#include "SpriteBatch.h"
#include "Texture.h"

namespace game
{
    PropertiesRef::PropertiesRef(gameplay::Properties * properties)
        : _properties(properties)
    {
    }

    PropertiesRef::~PropertiesRef()
    {
        SAFE_DELETE(_properties);
    }

    gameplay::Properties * PropertiesRef::get()
    {
        return _properties;
    }

    PropertiesRef * createProperties(const char * url)
    {
        PropertiesRef * result = nullptr;
        static std::map<std::string, PropertiesRef *> cache;

        auto itr = cache.find(url);
        if(itr == cache.end())
        {
            GAME_LOG("Loading properties '%s'", url);
            result = new PropertiesRef(gameplay::Properties::create(url));

            if(result)
            {
                cache[url] = result;
            }
            else
            {
                GAME_ASSERTFAIL("Failed to load proerties %s", url);
            }
        }
        else
        {
            result = itr->second;
            result->addRef();
        }

        return result;
    }

    gameplay::SpriteBatch * createSinglePixelSpritebatch()
    {
        std::array<unsigned char, 4> rgba;
        rgba.fill(std::numeric_limits<unsigned char>::max());
        gameplay::Texture * texture = gameplay::Texture::create(gameplay::Texture::Format::RGBA, 1, 1, &rgba.front());
        gameplay::SpriteBatch * spriteBatch = gameplay::SpriteBatch::create(texture);
        texture->release();
        return spriteBatch;
    }

#ifndef _FINAL
    /** @script{ignore} */
    struct LogDisplayMessage
    {
        gameplay::Logger::Level _level;
        char const * _message;
        int _secondsRemaining;
    };

    static std::deque<std::string> & getLogHistory()
    {
        static std::deque<std::string> logHistory;
        return logHistory;
    }
    
    /** @script{ignore} */
    struct LogRenderer
    {
        void render(void * message)
        {
            LogDisplayMessage * msg = static_cast<LogDisplayMessage*>(message);
            gameplay::Game::getInstance()->clear(gameplay::Game::CLEAR_COLOR_DEPTH, gameplay::Vector4::zero(), 1.0f, 0);
            gameplay::Font * font = gameplay::Font::create(gameplay::Game::getInstance()->getConfig()->getString("debug_font"));
            font->finish();
            int const margin = 10;
            int x = margin;
            int y = margin;
            int const yPadding = font->getSize();
            std::array<char, UCHAR_MAX> buffer;
            sprintf(&buffer[0], "%s! Resuming in %d", msg->_level == gameplay::Logger::LEVEL_ERROR ? "ERROR" : "WARNING", msg->_secondsRemaining);
            font->start();
            font->drawText(&buffer[0], x, y += yPadding, gameplay::Vector4(1, 0, 0, 1));
            font->drawText(msg->_message, x, y += yPadding, gameplay::Vector4::one());
            y += yPadding;
            font->drawText("Recent log history:", x, y += yPadding, gameplay::Vector4::one());
            y += yPadding;

            for (auto itr = getLogHistory().rbegin(); itr != getLogHistory().rend(); ++itr)
            {
                font->drawText(itr->c_str(), x, y += yPadding, gameplay::Vector4::fromColor(0xDBFF28FF));
            }

            font->finish();
        }
    };

    void clearLogHistory()
    {
        getLogHistory().clear();
    }
#endif

    void loggingCallback(gameplay::Logger::Level level, const char* msg)
    {
#ifndef _FINAL
        int const callsPerMessage = 3;
        static int callCount = 0;
        static std::string logOutput;
        static bool forceAssert = callCount == 0 && strcmp(msg, "lua assert:") == 0;
        
        if (level != gameplay::Logger::Level::LEVEL_INFO)
        {
            ++callCount;
            logOutput += msg;
            
            if (callCount == callsPerMessage)
            {
                gameplay::print(logOutput.c_str());

                int timeout = gameplay::Game::getInstance()->getConfig()->getInt("debug_assert_timeout_ms");

                if (timeout > 0)
                {
                    LogDisplayMessage message;
                    message._level = level;
                    message._message = logOutput.c_str();
                    LogRenderer renderer;
                    int const timeoutTick = 1000;

                    while (timeout > 0)
                    {
                        message._secondsRemaining = timeout / timeoutTick;
                        gameplay::ScreenDisplayer().run(&renderer, &LogRenderer::render, &message, timeoutTick);
                        timeout -= timeoutTick;
                    }

                    if (forceAssert)
                    {
                        GP_ASSERT(false);
                    }
                }

                logOutput.clear();
                callCount = 0;
                forceAssert = false;
            }
        }
        else
        {
            if (getLogHistory().size() == GAME_ON_SCREEN_LOG_HISTORY_CAPACITY)
            {
                getLogHistory().pop_front();
            }

            getLogHistory().push_back(msg);
            gameplay::print(msg);
        }
#else
        if (level != gameplay::Logger::Level::LEVEL_INFO)
        {
            // <Insert User LoadingScreenControllering crash notfication/log upload to a server>
        }
#endif
    }

    void forceReleaseRef(gameplay::Ref * ref)
    {
        if (ref)
        {
            while (true)
            {
                bool const done = ref->getRefCount() == 1;
                ref->release();

                if (done)
                {
                    break;
                }
            }
        }
    }
}
