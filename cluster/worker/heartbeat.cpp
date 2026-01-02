#include "heartbeat.h"
#include "../common/telemetry.h"
#include "telemetry_collector.h"
#include <unistd.h>

static std::string get_hostname() {
  char buf[256]{};
  if (gethostname(buf, sizeof(buf)-1) == 0) return std::string(buf);
  return "unknown";
}

namespace heartbeat {

std::string make_payload(TelemetryCollector& col) {
  Telemetry t = col.sample();

  std::string p = "{";
  p += "\"hostname\":\"" + get_hostname() + "\",";
  p += "\"telemetry\":" + t.to_json();
  p += "}";
  return p;
}

}
