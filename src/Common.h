#ifndef PLATFORMER_COMMON_H
#define PLATFORMER_COMMON_H

#include <array>
#include "Base.h"
#include "Game.h"
#include "Logger.h"
#include "Ref.h"

namespace gameplay
{
    class Properties;
    class SpriteBatch;
}

namespace platformer
{
    #define PLATFORMER_ASSERT(expression, ...) if (!(expression)) GP_ERROR(__VA_ARGS__)
    #define PLATFORMER_ASSERTFAIL(...) PLATFORMER_ASSERT(false, __VA_ARGS__)

    #define PLATFORMER_LOG(message, ...)\
    {\
        std::array<char, UCHAR_MAX> logTimeStamp;\
        sprintf(&logTimeStamp[0], "[%.2f] ", gameplay::Game::getAbsoluteTime() / 1000.0f);\
        gameplay::Logger::log(gameplay::Logger::Level::LEVEL_INFO, (std::string(&logTimeStamp[0]) + std::string(message) + "\n").c_str(), __VA_ARGS__);\
    }

    #define PLATFORMER_PRINT_VEC2(id, vec) PLATFORMER_LOG("%s: %f,%f", id, vec.x, vec.y)
    #define PLATFORMER_PRINT_VEC3(id, vec) PLATFORMER_LOG("%s: %f,%f,%f,", id, vec.x, vec.y, vec.z)
    #define PLATFORMER_RANDOM_RANGE_INT(min, max) min + (rand() % (int)(max - min + 1))
    #define PLATFORMER_FORCE_RELEASE(ref) forceReleaseRef(ref)
    #define PLATFORMER_ASSERT_SINGLE_REF(ref) PLATFORMER_ASSERT(ref->getRefCount() == 1, "Ref has references still outstanding")
    #define PLATFORMER_FONT_SIZE_SMALL 20
    #define PLATFORMER_FONT_SIZE_REGULAR 35
    #define PLATFORMER_FONT_SIZE_LARGE 50
    #define PLATFORMER_FONT_SIZE_REGULAR_INDEX 0
    #define PLATFORMER_FONT_SIZE_LARGE_INDEX 1
    #define PLATFORMER_MAX_GAMEPADS 1
    // 1 metre = 32 pixels
    #define PLATFORMER_UNIT_SCALAR 0.03125f

    /**
     * @script{ignore}
    */
    gameplay::SpriteBatch * createSinglePixelSpritebatch();

    class PropertiesRef : public gameplay::Ref
    {
    public:
        PropertiesRef(gameplay::Properties * properties);
        virtual ~PropertiesRef();
        gameplay::Properties * get();
    private:
        gameplay::Properties * _properties;
    };

    /**
     * @script{ignore}
    */
    PropertiesRef * createProperties(const char * url);

    /** @script{ignore} */
    void loggingCallback(gameplay::Logger::Level level, char const * msg);

    /**
     * Recursivley calls release() on the ref until it has been deleted
     *
     * @script{ignore}
    */
    void forceReleaseRef(gameplay::Ref * ref);
#ifndef _FINAL
    #define PLATFORMER_ON_SCREEN_LOG_HISTORY_CAPACITY 25

    /** @script{ignore} */
    void clearLogHistory();
#endif
}

#endif
