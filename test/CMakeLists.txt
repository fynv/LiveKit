cmake_minimum_required (VERSION 3.0)

add_executable(test_image test_image.cpp)
target_link_libraries(test_image LiveKit)

add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera LiveKit)

add_executable(test_window_capture test_window_capture.cpp)
target_link_libraries(test_window_capture LiveKit)

add_executable(test_window_record test_window_record.cpp)
target_link_libraries(test_window_record LiveKit)

install(TARGETS test_image test_camera test_window_capture test_window_record RUNTIME DESTINATION test_cpp)


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


install(FILES ${THIRD_PARTY_DLL} DESTINATION test_cpp)
