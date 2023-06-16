# Skatliste

Experimental Android app to capture images of handwritten tournament skat lists and check them for errors. One goal is to use only C/C++ without any Java code. Another goal is to use only simple and bloat-free external libraries which can be easily included and built with the project.

## Dependencies

* [rawdraw](https://github.com/cntools/rawdraw) Primitive Platform Agnostic Graphics Library
* [Dear ImGui](https://github.com/ocornut/imgui) Bloat-free Graphical User interface for C++ with minimal dependencies

## Building

* Install Android Studio
* Use the SDK Manager to install NDK and CMake
* Create a new Android Studio project with No Activity
* Clone this repository
* Replace the app folder in the new project by the cloned repository
* Use Anroid Studio to build the project

## Links

* [Getting started with C++ and Android Native Activities](https://medium.com/androiddevelopers/getting-started-with-c-and-android-native-activities-2213b402ffff)
* [Using Android Native Camera API (CPU & GPU Processing)](https://www.sisik.eu/blog/android/ndk/camera)
