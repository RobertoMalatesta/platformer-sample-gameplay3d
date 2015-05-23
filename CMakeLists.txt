cmake_minimum_required(VERSION 2.7)
PROJECT(platformer)

set(GAME_NAME platformer)
set(ARCH_DIR "x86_64" )

set(GAMEPLAY_SRC_PATH "external/GamePlay")
set(GAMEPLAY_EXT_LIBS_PATH "${GAMEPLAY_SRC_PATH}/external-deps")
set(GAMEPLAY_LIB_NAME "gameplay")
set(GAMEPLAY_LIB_DIR_NAME "GamePlay")
set(PLATFORMER_EXT_LIBS_PATH "external")
set(GAMEOBJECTS_LIB_NAME "gameobjects")
set(GAMEOBJECTS_LIB_DIR_NAME "gameobjects-gameplay3d")

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    ADD_DEFINITIONS(-D__linux__)
    SET(TARGET_OS "LINUX")
    SET(TARGET_OS_DIR "linux")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    ADD_DEFINITIONS(-D__APPLE__)
    SET(TARGET_OS "MACOSX")
    SET(TARGET_OS_DIR "macosx")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Windows")
    IF(MSVC)
        ADD_DEFINITIONS(-DMSVC)
    ENDIF(MSVC)
    ADD_DEFINITIONS(-DWIN32)
    ADD_DEFINITIONS(-D_WINDOWS)
    SET(TARGET_OS "WINDOWS")
    SET(TARGET_OS_DIR "windows")
ENDIF(CMAKE_SYSTEM_NAME MATCHES "Linux")

set(GAME_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/${TARGET_OS_DIR}")

macro (append_lib listToAppend libName libDirPath)
    IF (TARGET_OS STREQUAL "WINDOWS")
        FIND_LIBRARY(${libName}_LIBRARY_RELEASE
            NAMES ${libName}
            PATHS "${libDirPath}/${TARGET_OS_DIR}/${ARCH_DIR}/Release"
        )

        FIND_LIBRARY(${libName}_LIBRARY_DEBUG
            NAMES ${libName}
            PATHS "${libDirPath}/${TARGET_OS_DIR}/${ARCH_DIR}/Debug"
        )
        SET(FOUND_LIB_${libName}
            debug ${${libName}_LIBRARY_DEBUG}
            optimized ${${libName}_LIBRARY_RELEASE}
        )
    ELSE (TARGET_OS STREQUAL "WINDOWS")
        find_library(FOUND_LIB_${libName} ${libName} HINTS
            "${libDirPath}")
    ENDIF (TARGET_OS STREQUAL "WINDOWS")
    message(STATUS "Library Found: ${libName} Path: ${FOUND_LIB_${libName}}")
    set(${listToAppend} ${${listToAppend}} ${FOUND_LIB_${libName}})
endmacro(append_lib)

macro (append_gameplay_ext_lib listToAppend libName libDirName)
    IF("${libDirName}" STREQUAL "")
        find_library(FOUND_LIB_${libName} NAMES ${libName} ${ARGN})
    ELSE("${libDirName}" STREQUAL "")
        IF (TARGET_OS STREQUAL "WINDOWS")
            find_library(${libName}_LIBRARY_RELEASE
                NAMES ${libName} ${ARGN}
                PATHS "${GAMEPLAY_EXT_LIBS_PATH}/lib/${TARGET_OS_DIR}/${ARCH_DIR}/Release")
            find_library(${libName}_LIBRARY_DEBUG
                NAMES ${libName} ${ARGN}
                PATHS "${GAMEPLAY_EXT_LIBS_PATH}/lib/${TARGET_OS_DIR}/${ARCH_DIR}/Debug")

            set(FOUND_LIB_${libName}
                debug ${${libName}_LIBRARY_DEBUG}
                optimized ${${libName}_LIBRARY_RELEASE})
        ELSE (TARGET_OS STREQUAL "WINDOWS")
                set(pathToSearch
                    "${GAMEPLAY_EXT_LIBS_PATH}/lib/${TARGET_OS_DIR}/${ARCH_DIR}")
                find_library(FOUND_LIB_${libName} NAMES ${libName} ${ARGN} HINTS ${pathToSearch})
        ENDIF (TARGET_OS STREQUAL "WINDOWS")
    ENDIF("${libDirName}" STREQUAL "")

    set(${listToAppend} ${${listToAppend}} ${FOUND_LIB_${libName}})
    message(STATUS "Library Found: ${libName} Path: ${FOUND_LIB_${libName}}")
endmacro (append_gameplay_ext_lib)

macro(add_source_group SOURCE_PATH SOURCES)
    if(WIN32)
        STRING(REPLACE "/" "\\\\" SOURCE_GROUP_PATH ${SOURCE_PATH})
    endif()
    source_group(${SOURCE_GROUP_PATH} FILES ${SOURCES})
endmacro(add_source_group)

include_directories(
    ${GAMEPLAY_SRC_PATH}/gameplay/src
    ${CMAKE_SOURCE_DIR}/src
    ${GAMEPLAY_SRC_PATH}/external-deps/include
    ${PLATFORMER_EXT_LIBS_PATH}/${GAMEOBJECTS_LIB_DIR_NAME}/src
)

append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "gameplay-deps" "libs")

IF (NOT TARGET_OS STREQUAL "WINDOWS")
    append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "m" "" )
    append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "dl" "")    
    append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "pthread" "" )

    IF (TARGET_OS STREQUAL "LINUX")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "GL" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "X11" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "glib-2.0" "" )
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "rt" "" )
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "gtk-x11-2.0" "" )
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "gobject-2.0" "" )
    ELSE()
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "AppKit" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "CoreVideo" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "GameKit" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "Foundation" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "IOKit" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "OpenAL" "")
        append_gameplay_ext_lib(GAMEPLAY_LIBRARIES "OpenGL" "")
    ENDIF(TARGET_OS STREQUAL "LINUX")
ELSE()
    set(GAMEPLAY_LIBRARIES ${GAMEPLAY_LIBRARIES} "OpenGL32")
    set(GAMEPLAY_LIBRARIES ${GAMEPLAY_LIBRARIES} "GLU32")
    set(GAMEPLAY_LIBRARIES ${GAMEPLAY_LIBRARIES} "XInput")
    ADD_DEFINITIONS(-D_ITERATOR_DEBUG_LEVEL=2)
ENDIF (NOT TARGET_OS STREQUAL "WINDOWS")

# CMake recommends that you generate projects that are 'out of source' meaning that you create a 'build'
# folder in the top level directory that doesn't pollute the original src directory with project/CMake specific data.
# This is a good idea but it means the working directory of the game while debugging is now './build' instead of
# './' so the game won't be able to find any of the resources. The game shouldn't have to be aware of this nor do
# we want to have to copy any resource changes to the build folder everytime we change something. We can get around
# this by creating symlinks in the build folder that point the actual resources in the parent folder.
if(WIN32)
    execute_process(COMMAND cmd /C "${CMAKE_SOURCE_DIR}/raw/automation/mklink_win32.bat ${TARGET_OS_DIR}")
else()
    execute_process(COMMAND mkdir -p bin/${TARGET_OS_DIR}/Contents/Resources)
    execute_process(COMMAND ln -s ../res res)
    execute_process(COMMAND ln -s ../../../res bin/${TARGET_OS_DIR}/res)
    execute_process(COMMAND ln -s ../../../../../res bin/${TARGET_OS_DIR}/Contents/Resources/res)
    execute_process(COMMAND ln -s ../game.config game.config)
    execute_process(COMMAND ln -s ../user.config user.config)
    execute_process(COMMAND ln -s ../default_user.config default_user.config)
    execute_process(COMMAND ln -s ../../../game.config bin/${TARGET_OS_DIR}/game.config)
    execute_process(COMMAND ln -s ../../../user.config bin/${TARGET_OS_DIR}/user.config)
    execute_process(COMMAND ln -s ../../../default_user.config bin/${TARGET_OS_DIR}/default_user.config)
    execute_process(COMMAND ln -s ../../../../../game.config bin/${TARGET_OS_DIR}/Contents/Resources/game.config)
    execute_process(COMMAND ln -s ../../../../../user.config bin/${TARGET_OS_DIR}/Contents/Resources/user.config)
    execute_process(COMMAND ln -s ../../../../../default_user.config bin/${TARGET_OS_DIR}/Contents/Resources/default_user.config)
endif()

# A listing of all the project files
set(GAME_CONFIG ./build/game.config ./build/user.config ./build/default_user.config .gitignore)
set(ANDROID ./android/AndroidManifest.xml ./android/build.xml ./android/res/values/strings.xml ./android/jni/Android.mk ./android/jni/Application.mk)
file(GLOB RAW_LEVELS ./raw/levels/*)
file(GLOB RAW_SPRITESHEETS ./raw/spritesheets/*)
file(GLOB RAW_AUTOMATION ./raw/automation/*)
file(GLOB RES_AUDIO ./res/audio/*.audio)
file(GLOB RES_LUA ./res/lua/*.lua)
file(GLOB RES_LUA_TOOLS ./res/lua/tools/*)
file(GLOB RES_GAME_OBJECTS ./res/gameobjects/*)
file(GLOB RES_LEVELS ./res/levels/*)
file(GLOB RES_PARALLAX ./res/parallax/*)
file(GLOB RES_PHYSICS ./res/physics/*)
file(GLOB RES_SCENES ./res/scenes/*)
file(GLOB RES_SHADERS ./res/shaders/*)
file(GLOB RES_SPRITESHEETS ./res/spritesheets/*)
file(GLOB RES_UI ./res/ui/*)
file(GLOB GAME_SRC ./src/*.cpp ./src/*.h ./src/*.inl)

# How the project files will be presented in an IDE, not all of them support this, some will just
# display the file structure as it appears on disk
set(SG_ANDROID "android")
set(SG_GAME_SRC "src")
set(SG_GAME_CONFIG "res")
set(SG_RAW_LEVELS "raw/levels")
set(SG_RAW_AUTOMATION "raw/automation")
set(SG_RAW_SPRITESHEETS "raw/spritesheets")
set(SG_RES_AUDIO "res/audio")
set(SG_RES_LUA "res/lua")
set(SG_RES_LUA_TOOLS "res/lua/tools")
set(SG_RES_GAME_OBJECTS "res/gameobjects")
set(SG_RES_LEVELS "res/levels")
set(SG_RES_PARALLAX "res/parallax")
set(SG_RES_PHYSICS "res/physics")
set(SG_RES_SCENES "res/scenes")
set(SG_RES_SHADERS "res/shaders")
set(SG_RES_SPRITESHEETS "res/spritesheets")
set(SG_RES_UI "res/ui")

if(WIN32)

    # The behaviour for passing file list variables as macro arguments seems to differ
    # on Windows. The list gets truncated to include only its first entry and is converted
    # to a string without being expanded e.g. './res/*.txt' . Converting a list to a string
    # before passing it as an argument prevents truncation causes it to get fully expanded.

    add_source_group(${SG_ANDROID} "${ANDROID}")
    add_source_group(${SG_GAME_SRC} "${GAME_SRC}")
    add_source_group(${SG_GAME_CONFIG} "${GAME_CONFIG}")
    add_source_group(${SG_RAW_LEVELS} "${RAW_LEVELS}")
    add_source_group(${SG_RAW_AUTOMATION} "${RAW_AUTOMATION}")
    add_source_group(${SG_RAW_SPRITESHEETS} "${RAW_SPRITESHEETS}")
    add_source_group(${SG_RES_AUDIO} "${RES_AUDIO}")
    add_source_group(${SG_RES_LUA} "${RES_LUA}")
    add_source_group(${SG_RES_LUA_TOOLS} "${RES_LUA_TOOLS}")
    add_source_group(${SG_RES_GAME_OBJECTS} "${RES_GAME_OBJECTS}")
    add_source_group(${SG_RES_LEVELS} "${RES_LEVELS}")
    add_source_group(${SG_RES_PARALLAX} "${RES_PARALLAX}")
    add_source_group(${SG_RES_PHYSICS} "${RES_PHYSICS}")
    add_source_group(${SG_RES_SCENES} "${RES_SCENES}")
    add_source_group(${SG_RES_SPRITESHEETS} "${RES_SPRITESHEETS}")
    add_source_group(${SG_RES_SHADERS} "${RES_SHADERS}")
    add_source_group(${SG_RES_UI} "${RES_UI}")
else()
    add_source_group(${SG_ANDROID} ${ANDROID})
    add_source_group(${SG_GAME_SRC} ${GAME_SRC})
    add_source_group(${SG_GAME_CONFIG} ${GAME_CONFIG})
    add_source_group(${SG_RAW_LEVELS} ${RAW_LEVELS})
    add_source_group(${SG_RAW_AUTOMATION} ${RAW_AUTOMATION})
    add_source_group(${SG_RAW_SPRITESHEETS} ${RAW_SPRITESHEETS})
    add_source_group(${SG_RES_AUDIO} ${RES_AUDIO})
    add_source_group(${SG_RES_LUA} ${RES_LUA})
    add_source_group(${SG_RES_LUA_TOOLS} ${RES_LUA_TOOLS})
    add_source_group(${SG_RES_GAME_OBJECTS} ${RES_GAME_OBJECTS})
    add_source_group(${SG_RES_LEVELS} ${RES_LEVELS})
    add_source_group(${SG_RES_PARALLAX} ${RES_PARALLAX})
    add_source_group(${SG_RES_PHYSICS} ${RES_PHYSICS})
    add_source_group(${SG_RES_SCENES} ${RES_SCENES})
    add_source_group(${SG_RES_SPRITESHEETS} ${RES_SPRITESHEETS})
    add_source_group(${SG_RES_SHADERS} ${RES_SHADERS})
    add_source_group(${SG_RES_UI} ${RES_UI})
endif()

add_executable(${GAME_NAME}
    WIN32
    ${GAME_SRC}
    ${ANDROID}
    ${GAME_CONFIG}
    ${RES_GAME_OBJECTS}
    ${RES_SPRITESHEETS}
    ${RES_SCENES}
    ${RAW_SPRITESHEETS}
    ${RAW_LEVELS}
    ${RES_AUDIO}
    ${RES_LEVELS}
    ${RES_PARALLAX}
    ${RES_PHYSICS}
    ${RES_LUA}
    ${RES_LUA_TOOLS}
    ${RAW_AUTOMATION}
    ${RES_UI}
    ${RES_SHADERS}
)

if(MSVC)
    # Enable multi-processor compilation for faster compile times
    add_definitions(/MP)
    add_definitions(/Gm-)
    add_definitions(-DUNICODE)
    add_definitions(-DGP_USE_MEM_LEAK_DETECTION)
else()
    # Enable C++11
    set(CMAKE_CXX_FLAGS "-std=c++11")
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        add_definitions(-lstdc++)
    endif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_C_COMPILER_INIT g++)
endif()

set(GAMEPLAY_VERSION 3.0.0)
add_subdirectory(external/pngcrush)
add_subdirectory(external/json-to-gameplay3d)
add_subdirectory(${PLATFORMER_EXT_LIBS_PATH}/${GAMEPLAY_LIB_DIR_NAME}/gameplay)
add_subdirectory(${PLATFORMER_EXT_LIBS_PATH}/${GAMEOBJECTS_LIB_DIR_NAME})
add_dependencies(${GAME_NAME} ${GAMEPLAY_LIB_NAME} ${GAMEOBJECTS_LIB_NAME})
target_link_libraries(${GAME_NAME} ${GAMEPLAY_LIB_NAME} ${GAMEPLAY_LIBRARIES} ${GAMEOBJECTS_LIB_NAME})

set_target_properties(${GAME_NAME} PROPERTIES
    OUTPUT_NAME "${GAME_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${GAME_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${GAME_OUTPUT_DIR}"
)
