#ifndef LOG_FORMAT_H
#define LOG_FORMAT_H

#include <stdio.h>

// Shared log line format: [<millis>][<tag>] <message>
inline int formatLogLine(char* out, size_t out_size, unsigned long ms, const char* tag, const char* message) {
  return snprintf(out, out_size, "[%lu][%s] %s", ms, tag, message);
}

#endif  // LOG_FORMAT_H
