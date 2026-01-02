#pragma once
#include <cstdint>
#include <string>
#include <sstream>

struct Telemetry {
  uint64_t tsMs = 0;        // ölçüm zamanı
  float cpuTempC = 0.0f;    // cpu sıcaklığı
  float cpuUsagePct = 0.0f; // cpu kullanım %
  uint32_t memUsedMB = 0;   // kullanılan ram
  uint32_t memTotalMB = 0;  // toplam ram
  uint32_t uptimeSec = 0;   // uptime

  // Basit JSON üretimi (dependency yok)
  std::string to_json() const {
    std::ostringstream o;
    o << "{"
      << "\"tsMs\":" << tsMs << ","
      << "\"cpuTempC\":" << cpuTempC << ","
      << "\"cpuUsagePct\":" << cpuUsagePct << ","
      << "\"memUsedMB\":" << memUsedMB << ","
      << "\"memTotalMB\":" << memTotalMB << ","
      << "\"uptimeSec\":" << uptimeSec
      << "}";
    return o.str();
  }
};
