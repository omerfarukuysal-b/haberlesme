#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

class CommandBroker {
public:
  void store_response(uint32_t replyToSeq, const std::string& responseJson);

  // Wait until response arrives or timeoutMs; returns empty string on timeout.
  std::string wait_response(uint32_t cmdSeq, uint64_t timeoutMs);

private:
  std::mutex mu_;
  std::unordered_map<uint32_t, std::string> responses_;
};
