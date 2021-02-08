cmake_minimum_required (VERSION 3.0)
project(LiveKit)


set (INCLUDE_DIR
thirdparty/include
.
./internal
)

set (LIB_DIR
thirdparty/lib
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
internal/RenderingOGL.cpp
internal/SoundRecorder.cpp
VideoPort.cpp
ImageFile.cpp
Camera.cpp
WindowCapture.cpp
Viewer.cpp
Recorder.cpp
Compositor.cpp
)

set (LIB_HEADERS
VideoPort.h
ImageFile.h
Camera.h
WindowCapture.h
Viewer.h
Recorder.h
Compositor.h
)

set(INTERNAL_HEADERS
internal/Utils.h
internal/Image.h
internal/RenderingOGL.h
internal/BufferQueue.h
internal/SoundRecorder.h
)

add_definitions(${DEFINES})

add_library(LiveKit ${LIB_SOURCES} ${LIB_HEADERS} ${INTERNAL_HEADERS})
target_link_libraries(LiveKit avformat avdevice avcodec avfilter avutil postproc swscale swresample glew32 glfw3dll opengl32 strmiids winmm)

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
