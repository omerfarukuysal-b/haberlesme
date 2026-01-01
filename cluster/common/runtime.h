#pragma once
#include <atomic>

// Global run flag (defined once in a .cpp)
extern std::atomic<bool> g_running;