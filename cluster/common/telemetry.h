#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <map>

struct Telemetry
{
  uint64_t tsMs = 0;
  float cpuTempC = 0.0f;
  float cpuUsagePct = 0.0f;
  uint32_t memUsedMB = 0;
  uint32_t memTotalMB = 0;
  uint32_t uptimeSec = 0;

  // Dinamik mesaj alanı
  std::string message;

  std::map<uint8_t, uint32_t> txMap;
  std::map<uint8_t, uint32_t> rxMap;

  std::string map_to_json(const std::map<uint8_t, uint32_t> &m) const
  {
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto &kv : m)
    {
      if (!first)
        ss << ",";
      ss << "\"" << (int)kv.first << "\":" << kv.second;
      first = false;
    }
    ss << "}";
    return ss.str();
  }

  std::string to_json(int targetSize = 256) const
  {
    std::ostringstream o;
    o << "{"
      << "\"tsMs\":" << tsMs << ","
      << "\"cpuTempC\":" << cpuTempC << ","
      << "\"cpuUsagePct\":" << cpuUsagePct << ","
      << "\"memUsedMB\":" << memUsedMB << ","
      << "\"memTotalMB\":" << memTotalMB << ","
      << "\"uptimeSec\":" << uptimeSec << ","
      << "\"msg\":\"" << message << "\","
      // YENİ: Detaylı JSON çıktıları
      << "\"tx_details\":" << map_to_json(txMap) << ","
      << "\"rx_details\":" << map_to_json(rxMap);

    // Padding mantığı (Yaklaşık hesap)
    std::string current = o.str();
    int needed = targetSize - (int)current.size() - 10;
    if (needed > 0)
    {
      o << ",\"pad\":\"" << std::string(needed, 'X') << "\"";
    }

    o << "}";
    return o.str();
  }
};
