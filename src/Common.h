#ifndef GAME_COMMON_H
#define GAME_COMMON_H

#include <array>
#include "Base.h"

namespace game
{
    #define GAME_ASSERT(expression, ...) if (!(expression)) GP_ERROR(__VA_ARGS__)
    #define GAME_ASSERTFAIL(...) GAME_ASSERT(false, __VA_ARGS__)

    #define GAME_LOG(message, ...)\
    {\
        std::array<char, UCHAR_MAX> logTimeStamp;\
        sprintf(&logTimeStamp[0], "[%.2f] ", gameplay::Game::getAbsoluteTime() / 1000.0f);\
        gameplay::Logger::log(gameplay::Logger::Level::LEVEL_INFO, (std::string(&logTimeStamp[0]) + std::string(message) + "\n").c_str(), __VA_ARGS__);\
    }

    #define GAME_SAFE_ADD(ref) if(ref) ref->addRef()
    #define GAME_PRINT_VEC2(id, vec) GAME_LOG("%s: %f,%f", id, vec.x, vec.y)
    #define GAME_PRINT_VEC3(id, vec) GAME_LOG("%s: %f,%f,%f,", id, vec.x, vec.y, vec.z)
    #define GAME_PRINT_VEC4(id, vec) GAME_LOG("%s: %f,%f,%f,%f,", id, vec.x, vec.y, vec.z, vec.w)
    #define GAME_RANDOM_RANGE_INT(min, max) min + (rand() % (int)(max - min + 1))
    #define GAME_ASSERT_SINGLE_REF(ref) GAME_ASSERT(ref->getRefCount() == 1, "Ref has references still outstanding")
    #define GAME_FONT_SIZE_SMALL 20
    #define GAME_FONT_SIZE_REGULAR 35
    #define GAME_FONT_SIZE_LARGE 50
    #define GAME_FONT_SIZE_REGULAR_INDEX 0
    #define GAME_FONT_SIZE_LARGE_INDEX 1
    // 1 metre = 32 pixels
    #define GAME_UNIT_SCALAR 0.03125f

    struct StallScope
    {
        ~StallScope();
    };

    #define STALL_SCOPE() StallScope stallScope;

#ifndef _FINAL
    struct PerfScope
    {
        PerfScope(std::string const & id);
        ~PerfScope();

        double _start;
        std::string _id;
    };

    #define PERF_SCOPE(id) PerfScope perfScope(id)
#else
    #define PERF_SCOPE(id)
#endif

    /** @script{ignore} */
    void loggingCallback(gameplay::Logger::Level level, char const * msg);
#ifndef _FINAL
    #define GAME_ON_SCREEN_LOG_HISTORY_CAPACITY 25

    /** @script{ignore} */
    void clearLogHistory();
#endif
}

#endif
