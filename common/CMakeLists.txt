add_library(common
    logger.cpp
    sysinfo_utils.cpp
)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)
target_link_libraries(common
  PUBLIC
    spdlog::spdlog
    PkgConfig::SYSTEMD
    Boost::system
    Boost::thread
)
