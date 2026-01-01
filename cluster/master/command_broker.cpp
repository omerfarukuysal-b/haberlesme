#include "command_broker.h"
#include "../common/protocol.h"
#include <thread>

void CommandBroker::store_response(uint32_t replyToSeq, const std::string& responseJson) {
  std::lock_guard<std::mutex> lk(mu_);
  responses_[replyToSeq] = responseJson;
}

std::string CommandBroker::wait_response(uint32_t cmdSeq, uint64_t timeoutMs) {
  const uint64_t start = proto::now_ms();
  while (proto::now_ms() - start < timeoutMs) {
    {
      std::lock_guard<std::mutex> lk(mu_);
      auto it = responses_.find(cmdSeq);
      if (it != responses_.end()) {
        std::string out = it->second;
        responses_.erase(it);
        return out;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return "";
}
