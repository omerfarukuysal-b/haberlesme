#include "protocol.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <cstddef>

namespace proto {

uint64_t now_ms() {
  using namespace std::chrono;
  return (uint64_t)duration_cast<milliseconds>(
    steady_clock::now().time_since_epoch()).count();
}

static void write_u64_be(uint64_t v, uint8_t out[8]) {
  for (int i = 0; i < 8; i++) out[i] = (uint8_t)((v >> (56 - 8*i)) & 0xFF);
}
static uint64_t read_u64_be(const uint8_t in[8]) {
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) v = (v << 8) | (uint64_t)in[i];
  return v;
}

std::vector<uint8_t> encode(MsgType type,
                            uint8_t senderId,
                            uint32_t seq,
                            uint32_t replyToSeq,
                            const std::string& payload) {
  PacketHeader h{};
  h.magic    = htonl(MAGIC_RPI4);
  h.version  = VERSION;
  h.type     = (uint8_t)type;
  h.senderId = senderId;
  h.reserved = 0;
  h.seq      = htonl(seq);
  h.replyToSeq = htonl(replyToSeq);

  uint8_t tsbuf[8];
  write_u64_be(now_ms(), tsbuf);
  std::memcpy(&h.timestampMs, tsbuf, 8);

  h.payloadLen = htons((uint16_t)payload.size());

  std::vector<uint8_t> out(sizeof(PacketHeader) + payload.size());
  std::memcpy(out.data(), &h, sizeof(PacketHeader));
  if (!payload.empty()) {
    std::memcpy(out.data() + sizeof(PacketHeader), payload.data(), payload.size());
  }
  return out;
}

bool decode(const uint8_t* data, size_t len, Packet& out) {
  if (len < sizeof(PacketHeader)) return false;

  PacketHeader h{};
  std::memcpy(&h, data, sizeof(PacketHeader));

  if (ntohl(h.magic) != MAGIC_RPI4) return false;
  if (h.version != VERSION) return false;

  const uint16_t payLen = ntohs(h.payloadLen);
  if (sizeof(PacketHeader) + payLen > len) return false;

  out.hdr = h;

  // normalize some fields to host for convenience
  out.hdr.magic = MAGIC_RPI4;
  out.hdr.seq = ntohl(h.seq);
  out.hdr.replyToSeq = ntohl(h.replyToSeq);
  out.hdr.payloadLen = payLen;

  // timestamp read from original buffer (big endian)
  const uint8_t* tptr = data + offsetof(PacketHeader, timestampMs);
  out.hdr.timestampMs = read_u64_be(tptr);

  out.payload.assign((const char*)data + sizeof(PacketHeader),
                     (const char*)data + sizeof(PacketHeader) + payLen);
  return true;
}

} // namespace proto
