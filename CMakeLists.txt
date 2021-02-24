cmake_minimum_required (VERSION 3.0)
project(LiveKit)

SET(GLFW_BUILD_DOCS false)
SET(GLFW_INSTALL false)

add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glew-cmake)

SET(FFMPEG_ROOT ${FFMPEG_ROOT} CACHE PATH "FFMpeg path" FORCE)

set (INCLUDE_DIR
${FFMPEG_ROOT}/include
thirdparty/glfw/include
thirdparty/glew-cmake/include
.
./internal
)

set (LIB_DIR
${FFMPEG_ROOT}/lib
)


if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
-D"_CRT_SECURE_NO_WARNINGS"
)
else()
add_definitions(-std=c++17)
add_compile_options(-fPIC)
endif()

include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})

set (LIB_SOURCES
internal/Image.cpp
internal/AudioBuffer.cpp
internal/RenderingOGL.cpp
internal/AudioPort.cpp
internal/AudioIO.cpp
VideoPort.cpp
ImageFile.cpp
Player.cpp
LazyPlayer.cpp
Camera.cpp
WindowCapture.cpp
Viewer.cpp
Recorder.cpp
Compositor.cpp
IPCTarget.cpp
IPCSource.cpp
Copier.cpp
AudioRepeater.cpp
)

set (LIB_HEADERS
VideoPort.h
ImageFile.h
Player.h
LazyPlayer.h
Camera.h
WindowCapture.h
Viewer.h
Recorder.h
Compositor.h
IPCTarget.h
IPCSource.h
Copier.h
AudioRepeater.h
)

set(INTERNAL_HEADERS
internal/Utils.h
internal/Image.h
internal/AudioBuffer.h
internal/RenderingOGL.h
internal/AudioPort.h
internal/AudioIO.h
)

add_definitions(${DEFINES})

add_library(LiveKit ${LIB_SOURCES} ${LIB_HEADERS} ${INTERNAL_HEADERS})
target_link_libraries(LiveKit avformat avdevice avcodec avfilter avutil postproc swscale swresample libglew_static glfw opengl32 strmiids winmm)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX  ../install CACHE PATH "Install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

install(FILES ${LIB_HEADERS} DESTINATION include)
install(TARGETS LiveKit DESTINATION lib)

set(LIVEKIT_BUILD_TESTS false CACHE BOOL "Build tests")

if (LIVEKIT_BUILD_TESTS)
add_subdirectory(test)
endif()

set(BUILD_PYTHON_BINDINGS true CACHE BOOL "Build Python Bindings")

if (BUILD_PYTHON_BINDINGS)
add_subdirectory(python)
endif()
