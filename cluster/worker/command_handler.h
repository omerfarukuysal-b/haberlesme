#pragma once
#include <string>

class TelemetryCollector;

class CommandHandler {
public:
  // Constructor artık Collector referansı alıyor
  CommandHandler(TelemetryCollector& collector) : collector_(collector) {}

  std::string handle(const std::string& payload);

private:
  TelemetryCollector& collector_; // Referans
  static std::string get_hostname();
  static std::string get_uptime_sec();
};
