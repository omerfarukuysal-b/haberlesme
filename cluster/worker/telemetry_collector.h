#pragma once
#include "../common/telemetry.h"
#include <mutex>
class TelemetryCollector {
public:
  Telemetry sample();
  // YENİ: Mesajı güncellemek için metod
  void set_message(const std::string& msg);

private:
  std::mutex mu_;
  std::string current_msg_ = "Ready"; // Varsayılan mesaj
};
