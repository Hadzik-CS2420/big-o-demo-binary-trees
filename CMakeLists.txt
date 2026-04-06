cmake_minimum_required(VERSION 3.24)
project(big-o-demo-binary-trees LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} src/main.cpp)
add_executable(${PROJECT_NAME}-space src/space_main.cpp)

# Pass repo root so executables can find graph scripts and write CSV there
target_compile_definitions(${PROJECT_NAME} PRIVATE
    REPO_DIR="${CMAKE_SOURCE_DIR}"
)
target_compile_definitions(${PROJECT_NAME}-space PRIVATE
    REPO_DIR="${CMAKE_SOURCE_DIR}"
)
