cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_SYSTEM_NAME  Android)

set(CMAKE_SYSTEM_VERSION 1)

#SET(CMAKE_BUILD_TYPE Debug)
#SET(CMAKE_BUILD_TYPE RelWithDebInfo)
SET(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} -Ofast -funwind-tables -Wl,--no-merge-exidx-entries -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Ofast -funwind-tables -Wl,--no-merge-exidx-entries -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Ofast -funwind-tables -Wl,--no-merge-exidx-entries -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} -Ofast -funwind-tables -Wl,--no-merge-exidx-entries -std=c++11 -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Ofast -funwind-tables -Wl,--no-merge-exidx-entries -std=c++11 -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -funwind-tables -Wl,--no-merge-exidx-entries -std=c++11 -march=armv7-a -mfpu=neon -funsafe-math-optimizations -ffp-contract=fast -freciprocal-math -fno-signed-zeros")

set(NE10_ASM_OPTIMIZATION on)

set(FLOAT_ABI "hard")

set(PREBUILT_DIR ${CMAKE_SOURCE_DIR}/../prebuilt)

add_library(main SHARED
    src/main/c/coffeecatch.c
    src/main/cpp/coffeejni.cpp

    src/main/cpp/exceptionUtils.cpp
    src/main/c/generalUtils.c

    src/main/cpp/JNIHandler.cpp
    src/main/cpp/AssetManager.cpp
    src/main/cpp/Render.cpp
    src/main/cpp/Physics.cpp
    src/main/cpp/InputManager.cpp
    src/main/cpp/Engine.cpp)

target_include_directories(main PRIVATE
                           ${PREBUILT_DIR}/include
                           ${PREBUILT_DIR}/include/eigen
                           ./src/main/cpp
                           ./src/main/c)

target_link_libraries(main
                      log
                      z
                      android
                      EGL
                      GLESv2)