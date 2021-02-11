cmake_minimum_required (VERSION 3.0)

add_executable(test_image test_image.cpp)
target_link_libraries(test_image LiveKit)

add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera LiveKit)

add_executable(test_window_capture test_window_capture.cpp)
target_link_libraries(test_window_capture LiveKit)

add_executable(test_window_record test_window_record.cpp)
target_link_libraries(test_window_record LiveKit)

add_executable(test_compositor test_compositor.cpp)
target_link_libraries(test_compositor LiveKit)

install(TARGETS test_image test_camera test_window_capture test_window_record test_compositor RUNTIME DESTINATION test_cpp)
