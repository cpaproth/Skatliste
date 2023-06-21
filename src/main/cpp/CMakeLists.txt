cmake_minimum_required(VERSION 3.6.0)
project(Skatliste)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(external)
add_library(${PROJECT_NAME} SHARED main.cpp NdkCam.cpp)
target_link_libraries(${PROJECT_NAME} android native_app_glue GLESv3 EGL log camera2ndk mediandk)
