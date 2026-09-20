cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(educore)

set(ADDITIONAL_FILES "")

if (APPLE)
    set(ADDITIONAL_FILES catedu/impl.m)
endif()

file(GLOB_RECURSE CATEDU_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/catedu/*.cpp" ${ADDITIONAL_FILES} "lib/lib-enet/*.c" "lib/umka/src/*.c")
file(GLOB IMGUI_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/*.cpp")
list(APPEND CATEDU_SRC_FILES ${IMGUI_SRC_FILES})
list(REMOVE_ITEM CATEDU_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/umka/src/umka.c)

if (${ENV_TYPE} MATCHES "UNITTEST")
    add_compile_definitions(RUN_TESTS)
    list(REMOVE_ITEM CATEDU_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/catedu/main.cpp)
    list(APPEND CATEDU_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/test_main.cpp)
endif()

if (${ENV_TYPE} MATCHES "TESTBED")
    list(REMOVE_ITEM CATEDU_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/catedu/main.cpp)
    list(APPEND CATEDU_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/testbed/${TESTBED_NAME}/tb_main.cpp)
endif()

add_executable(${PROJECT_NAME} ${CATEDU_SRC_FILES})

if (${ENV_TYPE} MATCHES "RUNTIME")
    target_compile_definitions(${PROJECT_NAME} PUBLIC RUNTIME_MODE)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC UMKA_STATIC UMKA_EXT_LIBS)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC d3d11 ws2_32 winmm)
endif()

if (APPLE)
    find_library(QUARTZ_CORE QuartzCore)
    find_library(METAL Metal)
    find_library(METAL_KIT MetalKit)
    find_library(COCOA_LIBRARY Cocoa ONLY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY} ${METAL} ${METAL_KIT} ${QUARTZ_CORE})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    #target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=leak)
    target_link_libraries(${PROJECT_NAME} PUBLIC GL X11 pthread dl Xcursor Xi)# asan)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ./)
target_include_directories(${PROJECT_NAME} PUBLIC lib/)
target_include_directories(${PROJECT_NAME} PUBLIC lib/tau)
target_include_directories(${PROJECT_NAME} PUBLIC lib/umka/src)
target_include_directories(${PROJECT_NAME} PUBLIC lib/lib-enet/include)
target_include_directories(${PROJECT_NAME} PUBLIC lib/imgui)
