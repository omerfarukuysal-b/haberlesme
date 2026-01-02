#include "telemetry_collector.h"
#include "common/protocol.h"
#include <unistd.h>
#include <cstdio>
#include <string>

// /proc/uptime
static uint32_t get_uptime_sec() {
  FILE* f = std::fopen("/proc/uptime", "r");
  if (!f) return 0;
  double up = 0.0;
  std::fscanf(f, "%lf", &up);
  std::fclose(f);
  return (uint32_t)up;
}

// mem info (kB)
static void get_mem_mb(uint32_t& used, uint32_t& total) {
  FILE* f = std::fopen("/proc/meminfo", "r");
  if (!f) { used = total = 0; return; }

  long memTotalKB=0, memAvailKB=0;
  char key[64]; long val; char unit[32];

  while (std::fscanf(f, "%63s %ld %31s\n", key, &val, unit) == 3) {
    if (std::string(key) == "MemTotal:") memTotalKB = val;
    else if (std::string(key) == "MemAvailable:") memAvailKB = val;
  }
  std::fclose(f);

  total = (uint32_t)(memTotalKB / 1024);
  uint32_t avail = (uint32_t)(memAvailKB / 1024);
  used = (total > avail) ? (total - avail) : 0;
}

// cpu temp (C) - Pi'de genelde var
static float get_cpu_temp_c() {
  FILE* f = std::fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  if (!f) return 0.0f;
  long milli = 0;
  std::fscanf(f, "%ld", &milli);
  std::fclose(f);
  return (float)milli / 1000.0f;
}

// CPU usage% (çok basit: /proc/stat iki örnek arası)
static bool read_cpu_times(uint64_t& idle, uint64_t& total) {
  FILE* f = std::fopen("/proc/stat", "r");
  if (!f) return false;
  char cpu[8];
  uint64_t user,nice,sys,idleT,iowait,irq,softirq,steal;
  int n = std::fscanf(f, "%7s %lu %lu %lu %lu %lu %lu %lu %lu",
                      cpu,&user,&nice,&sys,&idleT,&iowait,&irq,&softirq,&steal);
  std::fclose(f);
  if (n < 5) return false;
  idle = idleT + iowait;
  total = user + nice + sys + idleT + iowait + irq + softirq + steal;
  return true;
}

Telemetry TelemetryCollector::sample() {
  Telemetry t;
  t.tsMs = proto::now_ms();
  t.uptimeSec = get_uptime_sec();

  get_mem_mb(t.memUsedMB, t.memTotalMB);
  t.cpuTempC = get_cpu_temp_c();

  // usage estimate
  uint64_t idle1=0,total1=0, idle2=0,total2=0;
  if (read_cpu_times(idle1,total1)) {
    usleep(50 * 1000); // 50ms
    if (read_cpu_times(idle2,total2) && total2 > total1) {
      uint64_t dTotal = total2 - total1;
      uint64_t dIdle  = idle2 - idle1;
      double usage = (dTotal > 0) ? (double)(dTotal - dIdle) * 100.0 / (double)dTotal : 0.0;
      t.cpuUsagePct = (float)usage;
    }
  }

  return t;
}
