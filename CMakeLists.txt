cmake_minimum_required(VERSION 3.25)
project(SdlDsuServer CXX)


find_package(SDL2 REQUIRED)
find_package(asio REQUIRED)
find_package(crc32c CONFIG REQUIRED)

add_executable(SdlDsuServer main.cpp cemuhook_controller_data.cpp crc32.cpp udp_server.cpp sdl_handler.cpp)
target_link_libraries(SdlDsuServer ${SDL2_LIBRARIES} asio::asio Crc32c::crc32c)

target_compile_features(SdlDsuServer PRIVATE cxx_std_17)