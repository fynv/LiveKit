cmake_minimum_required (VERSION 3.0)

project(PyLiveKit)

set (SRC
api.cpp
)

set (INCLUDE_DIR 
.
..
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
add_definitions(${DEFINES})

add_library(PyLiveKit SHARED ${SRC})
target_link_libraries(PyLiveKit LiveKit)


if (WIN32) 
install(TARGETS PyLiveKit RUNTIME DESTINATION test_python)
else()
install(TARGETS PyLiveKit DESTINATION test_python)
endif()

set(PYTHON
LiveKit.py
test_image.py
test_camera.py
test_window_capture.py
test_window_record.py
test_compositor.py

sign_green.png
sign_red.png
)

install(FILES ${PYTHON} DESTINATION test_python)


set(THIRD_PARTY_DLL 
../thirdparty/bin/avcodec-58.dll
../thirdparty/bin/avdevice-58.dll
../thirdparty/bin/avfilter-7.dll
../thirdparty/bin/avformat-58.dll
../thirdparty/bin/avutil-56.dll
../thirdparty/bin/postproc-55.dll
../thirdparty/bin/swresample-3.dll
../thirdparty/bin/swscale-5.dll
../thirdparty/bin/glew32.dll
../thirdparty/bin/glfw3.dll
)

install(FILES ${THIRD_PARTY_DLL} DESTINATION test_python)
