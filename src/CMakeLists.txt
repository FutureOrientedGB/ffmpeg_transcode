cmake_minimum_required(VERSION 3.20)


set(PROJECT_NAME transcode)


project(${PROJECT_NAME})


# find zlib
find_package(ZLIB REQUIRED)

# find fmt
find_package(fmt CONFIG REQUIRED)

# find spdlog
find_package(spdlog CONFIG REQUIRED)

# find ffmpeg
find_package(FFMPEG REQUIRED)

# find cli11
find_package(CLI11 CONFIG REQUIRED)


# defines
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ADD_DEFINITIONS(-DVC_EXTRALEAN)
ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)


# classify filters
FILE(GLOB_RECURSE HEADER_FILES
        "*.hpp"
)
FILE(GLOB_RECURSE SOURCE_FILES
        "*.cpp"
)
SOURCE_GROUP("Header Files" FILES ${HEADER_FILES})
SOURCE_GROUP("Source Files" FILES ${SRC_FILES})


# executable
add_executable(${PROJECT_NAME}
        WIN32
        ${HEADER_FILES}
        ${SOURCE_FILES}
)


# output dir
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $ENV{OUTPUT_EXE_DIR}
)


# Visual Studio - Properity - Linker - System - SubSystem > Console
if(MSVC)
set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)
endif(MSVC)


# Visual Studio - Properity - C/C++ - Code Generation - Rutime Library > /MT
if(MSVC)
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
endif(MSVC)


# Visual Studio - Properity - C/C++ - General - Additional Include Directories
target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        $ENV{FFMPEG_INCLUDE_DIRS}
)


# Visual Stuido - Properify - Linker - General - Additional Library Directories
link_directories(
        ${PROJECT_NAME}
        PRIVATE
        $ENV{FFMPEG_LIBRARY_DIRS}
)


# Visual Studio - Properity - C/C++ - General - Additional Link Libraries
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        # fmt
        fmt::fmt
        # spdlog
        spdlog::spdlog
        # cli11
        CLI11::CLI11
        # ffmpeg
        ${FFMPEG_LIBRARIES}
)
