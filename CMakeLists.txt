cmake_minimum_required(VERSION 3.20)


# project
project(ffmpeg_transcode)


# msvc
if(MSVC)
    # utf-8 source code encoding
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

    # Visual Stuido - Properity - C/C++ - General - Debug Informat Format > Program Database (/Zi) 
    set(CMAKE_CXX_FLAGS_DEBUUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")  # Disable Optimization
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O1")  # Favor Size

    # Visual Stuido - Properity - Linker - Debugging - Generate Debug Info > Generate Debug Information (/Debug)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

    # Visual Stuido - Properity - Linker - Debugging - Generate Debug Info > Generate Debug Information (/Debug)
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif(MSVC)


# projects
add_subdirectory(src)

