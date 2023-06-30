cmake_minimum_required(VERSION 3.6.0)
project(Skatliste)

add_library(rawdraw STATIC ext/CNFG.c ext/CNFGEGLDriver.c ext/CNFGFunctions.c)

add_library(imgui STATIC ext/imgui.cpp ext/imgui_draw.cpp ext/imgui_tables.cpp ext/imgui_demo.cpp
		ext/imgui_widgets.cpp ext/imgui_impl_android.cpp ext/imgui_impl_opengl3.cpp)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(ext ${ANDROID_NDK}/sources/android/native_app_glue)

add_library(${PROJECT_NAME} SHARED main.cpp Program.cpp NdkCam.cpp ListProc.cpp)
target_link_libraries(${PROJECT_NAME} android native_app_glue GLESv3 EGL log camera2ndk mediandk imgui)
