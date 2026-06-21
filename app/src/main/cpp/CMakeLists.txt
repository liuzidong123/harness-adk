# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("snakeshot")

# Optional: host-side Google Test for button-menu white-box tests
# Build with: cmake -DBUILD_TESTS=ON .. && make test_button_menu
option(BUILD_TESTS "Build host-side white-box tests" OFF)

if(NOT BUILD_TESTS)
    # Creates your game shared library. The name must be the same as the
    # one used for loading in your Kotlin/Java or AndroidManifest.txt files.
    add_library(snakeshot SHARED
            main.cpp
            AndroidOut.cpp
            Renderer.cpp
            Shader.cpp
            TextureAsset.cpp
            BitmapFont.cpp
            Utility.cpp)

    # Searches for a package provided by the game activity dependency
    find_package(game-activity REQUIRED CONFIG)

    # Forces the linker to keep the JNI entry point for GameActivity
    set(CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode -Wl,-z,max-page-size=16384")

    # Configure libraries CMake uses to link your target library.
    target_link_libraries(snakeshot
            # The game activity
            game-activity::game-activity_static

            # EGL and other dependent libraries required for drawing
            # and interacting with Android system
            EGL
            GLESv3
            jnigraphics
            android
            log)
else()
    enable_testing()
    set(NDK_GTEST_SRC "C:/Users/28098/AppData/Local/Android/Sdk/ndk/27.0.12077973/sources/third_party/googletest")
    set(GOOGLETEST_VERSION "1.14.0")
    add_subdirectory(${NDK_GTEST_SRC} ${CMAKE_BINARY_DIR}/gtest)
    add_executable(test_button_menu ../../test/cpp/test_button_menu.cpp)
    target_link_libraries(test_button_menu gtest gtest_main)
    target_include_directories(test_button_menu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(test_button_menu PUBLIC cxx_std_17)
    add_test(NAME button_menu COMMAND test_button_menu)
endif()