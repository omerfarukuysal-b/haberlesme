#pragma once
#include <string>

class CommandHandler {
public:
  // payload = {"cmdId":123,"action":"hostname","arg":"x"}
  // returns response JSON: {"cmdId":123,"ok":true,"result":"..."}
  std::string handle(const std::string& payload);

private:
  static std::string get_hostname();
  static std::string get_uptime_sec();
};
