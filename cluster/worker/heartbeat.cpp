#include "heartbeat.h"
#include <unistd.h>
#include <cstdio>
#include <string>

static std::string get_hostname() {
  char buf[256]{};
  if (gethostname(buf, sizeof(buf)-1) == 0) return std::string(buf);
  return "unknown";
}
static std::string get_uptime_sec() {
  FILE* f = std::fopen("/proc/uptime", "r");
  if (!f) return "0";
  double up = 0.0;
  std::fscanf(f, "%lf", &up);
  std::fclose(f);
  long sec = (long)up;
  return std::to_string(sec);
}

namespace heartbeat {
std::string make_payload() {
  std::string p = "{";
  p += "\"hostname\":\"" + get_hostname() + "\",";
  p += "\"uptimeSec\":" + get_uptime_sec();
  p += "}";
  return p;
}
}
