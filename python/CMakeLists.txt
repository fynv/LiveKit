cmake_minimum_required (VERSION 3.0)

project(PyLiveKit)

add_custom_target(Run_CFFIBuild
COMMAND python LiveKit/cffi_build.py
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
COMMENT "Running cffi_build")


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
install(TARGETS PyLiveKit RUNTIME DESTINATION test_python/LiveKit)
else()
install(TARGETS PyLiveKit DESTINATION test_python/LiveKit)
endif()

set(PYTHON
LiveKit/__init__.py
LiveKit/cffi.py
)

install(FILES ${PYTHON} DESTINATION test_python/LiveKit)
install(FILES setup.py DESTINATION test_python)

add_subdirectory(test)

