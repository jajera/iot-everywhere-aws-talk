#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <stdint.h>

#include <Arduino.h>

class NTPSync {
 public:
  void begin(const char* server);
  bool loop();
  bool isSynced() const;
  uint32_t getEpoch() const;
  int32_t getClockOffsetMs() const;
  void resetSync();
  void markSynced(int32_t offset_ms);

 private:
  const char* server_ = nullptr;
  bool started_ = false;
  bool synchronized_ = false;
  bool timeout_logged_ = false;
  int32_t clock_offset_ms_ = 0;
  uint32_t start_ms_ = 0;
  uint32_t last_poll_ms_ = 0;
};

#endif  // NTP_SYNC_H
