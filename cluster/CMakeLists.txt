cmake_minimum_required(VERSION 3.10)
project(rpi_cluster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type (optional)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Warnings (optional but nice)
add_compile_options(-Wall -Wextra -Wpedantic)

# ---- Common library ----
add_library(cluster_common
  common/protocol.cpp
  common/net_utils.cpp
  common/node_config.cpp
  common/command_dispatcher.cpp
)

target_include_directories(cluster_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# pthread
find_package(Threads REQUIRED)
target_link_libraries(cluster_common PUBLIC Threads::Threads)

# ---- Master executable ----
add_executable(master_router
  master/master_router_main.cpp
  master/node_registry.cpp
  master/command_broker.cpp
  master/udp_listener.cpp
  master/ipc_server.cpp
)

target_link_libraries(master_router PRIVATE cluster_common)

# ---- Worker executable ----
add_executable(worker_agent
  worker/worker_agent_main.cpp
  worker/udp_agent.cpp
  worker/heartbeat.cpp
  worker/command_handler.cpp
  worker/telemetry_collector.cpp
)

target_link_libraries(worker_agent PRIVATE cluster_common)
