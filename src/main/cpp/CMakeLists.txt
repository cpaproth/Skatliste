cmake_minimum_required(VERSION 3.6.0)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(Skatliste SHARED main.cpp)

target_link_libraries(Skatliste android native_app_glue GLESv3 EGL log camera2ndk mediandk)
