#pragma once
#include <cstdint>
#include <string>
#include <sstream>

struct Telemetry {
  uint64_t tsMs = 0;
  float cpuTempC = 0.0f;
  float cpuUsagePct = 0.0f;
  uint32_t memUsedMB = 0;
  uint32_t memTotalMB = 0;
  uint32_t uptimeSec = 0;
  
  // YENİ: Dinamik mesaj alanı
  std::string message; 

  std::string to_json() const {
    std::ostringstream o;
    o << "{"
      << "\"tsMs\":" << tsMs << ","
      << "\"cpuTempC\":" << cpuTempC << ","
      << "\"cpuUsagePct\":" << cpuUsagePct << ","
      << "\"memUsedMB\":" << memUsedMB << ","
      << "\"memTotalMB\":" << memTotalMB << ","
      << "\"uptimeSec\":" << uptimeSec << ","
      << "\"message\":\"" << message << "\""  // JSON'a ekledik
      << "}";
    return o.str();
  }
};
