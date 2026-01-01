#include "command_handler.h"
#include "common/mini_json.h"

#include <unistd.h>
#include <cstdio>

std::string CommandHandler::get_hostname() {
  char buf[256]{};
  if (gethostname(buf, sizeof(buf)-1) == 0) return std::string(buf);
  return "unknown";
}

std::string CommandHandler::get_uptime_sec() {
  FILE* f = std::fopen("/proc/uptime", "r");
  if (!f) return "0";
  double up = 0.0;
  std::fscanf(f, "%lf", &up);
  std::fclose(f);
  long sec = (long)up;
  return std::to_string(sec);
}

std::string CommandHandler::handle(const std::string& payload) {
  const std::string cmdId  = minijson::get(payload, "cmdId");
  const std::string action = minijson::get(payload, "action");
  const std::string arg    = minijson::get(payload, "arg");

  bool ok = true;
  std::string result;

  // allowlist
  if (action == "echo") {
    result = arg;
  } else if (action == "hostname") {
    result = get_hostname();
  } else if (action == "uptime") {
    result = get_uptime_sec();
  } else {
    ok = false;
    result = "unsupported action";
  }

  std::string resp = "{";
  resp += "\"cmdId\":" + (cmdId.empty() ? "0" : cmdId) + ",";
  resp += "\"ok\":" + std::string(ok ? "true" : "false") + ",";
  resp += "\"result\":\"" + result + "\"";
  resp += "}";
  return resp;
}
