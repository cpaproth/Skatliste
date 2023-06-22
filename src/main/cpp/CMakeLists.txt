cmake_minimum_required(VERSION 3.6.0)
project(Skatliste)

add_library(rawdraw STATIC external/CNFG.c external/CNFGEGLDriver.c external/CNFGFunctions.c)

add_library(imgui STATIC external/imgui.cpp external/imgui_draw.cpp external/imgui_tables.cpp external/imgui_demo.cpp
        external/imgui_widgets.cpp external/imgui_impl_android.cpp external/imgui_impl_opengl3.cpp)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(external)
add_library(${PROJECT_NAME} SHARED main.cpp NdkCam.cpp)
target_link_libraries(${PROJECT_NAME} android native_app_glue GLESv3 EGL log camera2ndk mediandk imgui)
