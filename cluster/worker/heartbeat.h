#pragma once
#include <string>
class TelemetryCollector;

namespace heartbeat {
std::string make_payload(TelemetryCollector& col);
}
