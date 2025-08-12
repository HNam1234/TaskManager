cmake_minimum_required(VERSION 3.16)

project(TreeManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(spdlog)

add_executable(treeManagement
    main.cpp
    logger.h
    logger.cpp
)

# Link all dependencies
target_link_libraries(treeManagement
    spdlog::spdlog
)
include(GNUInstallDirs)
install(TARGETS treeManagement
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
