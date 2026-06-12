cmake_minimum_required(VERSION 3.16)

project(BBTAN)

set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    ptsd
    URL         https://github.com/ntut-open-source-club/practical-tools-for-simple-design/archive/refs/tags/v0.2.zip
    SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/PTSD
)
FetchContent_MakeAvailable(ptsd)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(SRC_FILES
    ${SRC_DIR}/App.cpp
    ${SRC_DIR}/AppUpdate.cpp
    ${SRC_DIR}/AppUtil.cpp
    ${SRC_DIR}/Ball.cpp      
    ${SRC_DIR}/Brick.cpp     
    ${SRC_DIR}/Launcher.cpp  
    ${SRC_DIR}/LevelManager.cpp  
    ${SRC_DIR}/Prop.cpp
    ${SRC_DIR}/EffectLine.cpp
)

set(HEADER_FILES
    ${INCLUDE_DIR}/App.hpp
    ${INCLUDE_DIR}/Ball.hpp      
    ${INCLUDE_DIR}/Brick.hpp     
    ${INCLUDE_DIR}/Launcher.hpp  
    ${INCLUDE_DIR}/LevelManager.hpp
    ${INCLUDE_DIR}/Prop.hpp
    ${INCLUDE_DIR}/GameConfig.hpp
    ${INCLUDE_DIR}/DeathEffect.hpp
    ${INCLUDE_DIR}/EffectLine.hpp
)

add_executable(${PROJECT_NAME}
    ${SRC_DIR}/main.cpp
    ${SRC_FILES}
    ${HEADER_FILES}
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources")
else()
    message(FATAL_ERROR "Please use `-DCMAKE_BUILD_TYPE=Debug` build for now.")
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PTSD/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2main
    PTSD
)