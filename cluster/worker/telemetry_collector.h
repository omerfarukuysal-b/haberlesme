#pragma once
#include "../common/telemetry.h"
#include <mutex>
#include <map>
#include <string>

class TelemetryCollector {
public:
  Telemetry sample();
  // Mesajı güncellemek için metod
  void set_message(const std::string& msg);

  // Parametreli sayaçlar
  void inc_tx(uint8_t targetId);
  void inc_rx(uint8_t senderId);


private:
  std::mutex mu_;
  std::string current_msg_ = "Ready"; // Varsayılan mesaj

  // Map kullanıyoruz (Atomic map olmadığı için mutex ile koruyacağız)
  std::map<uint8_t, uint32_t> tx_map_;
  std::map<uint8_t, uint32_t> rx_map_;

};
