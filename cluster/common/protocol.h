#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace proto {

static constexpr uint32_t MAGIC_RPI4 = 0x52504934u; // "RPI4"
static constexpr uint8_t  VERSION   = 1;

enum class MsgType : uint8_t {
  Heartbeat = 10,
  Command   = 20,
  Response  = 21
};

#pragma pack(push, 1)
struct PacketHeader {
  uint32_t magic;       // network order on wire
  uint8_t  version;
  uint8_t  type;
  uint8_t  senderId;
  uint8_t  reserved;
  uint32_t seq;         // network order on wire
  uint32_t replyToSeq;  // network order on wire
  uint64_t timestampMs; // big-endian on wire
  uint16_t payloadLen;  // network order on wire
};
#pragma pack(pop)

struct Packet {
  PacketHeader hdr{};
  std::string payload; // UTF-8 JSON string
};

uint64_t now_ms();

// Encode a packet to bytes (wire format)
std::vector<uint8_t> encode(MsgType type,
                            uint8_t senderId,
                            uint32_t seq,
                            uint32_t replyToSeq,
                            const std::string& payload);

// Decode bytes into packet (returns false if invalid)
bool decode(const uint8_t* data, size_t len, Packet& out);

} // namespace proto
