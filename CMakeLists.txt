cmake_minimum_required(VERSION 3.16)

project(taskManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Conan setup
include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake OPTIONAL RESULT_VARIABLE _conan_toolchain_included)
include(${CMAKE_BINARY_DIR}/conan_deps.cmake OPTIONAL)

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(spdlog REQUIRED)

add_subdirectory(common)

add_executable(taskManager
    main.cpp
)

target_link_libraries(taskManager
    PRIVATE
    Boost::system
    spdlog::spdlog
    common
)

include(GNUInstallDirs)
install(TARGETS taskManager
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
