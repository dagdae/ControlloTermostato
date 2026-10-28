// Storico delle temperature (per il grafico) e registro degli eventi, in RAM.
#pragma once

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

template <int N>
class History {
 public:
  static constexpr int16_t kInvalid = INT16_MIN;
  struct Point {
    int16_t tempX10;  // decimi di grado, kInvalid se non disponibile
    uint8_t flags;    // bit0 riscaldamento consentito, bit2-3 stato, bit4-5 modalita'
  };

  void add(bool valid, float tempC, bool allowed, uint8_t state, uint8_t mode) {
    Point& p = buf_[head_];
    p.tempX10 = valid && !isnan(tempC) ? static_cast<int16_t>(lroundf(tempC * 10.0f)) : kInvalid;
    p.flags = static_cast<uint8_t>((allowed ? 1 : 0) | ((state & 3) << 2) | ((mode & 3) << 4));
    head_ = (head_ + 1) % N;
    if (count_ < N) ++count_;
  }
  int size() const { return count_; }
  // i = 0 e' il punto piu' vecchio.
  const Point& at(int i) const { return buf_[(head_ - count_ + i + N) % N]; }

 private:
  Point buf_[N];
  int head_ = 0;
  int count_ = 0;
};

template <int N>
class EventLog {
 public:
  struct Event {
    uint64_t ms;
    char text[80];
  };

  __attribute__((format(printf, 3, 4))) void add(uint64_t ms, const char* fmt, ...) {
    Event& e = buf_[head_];
    e.ms = ms;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(e.text, sizeof(e.text), fmt, ap);
    va_end(ap);
    head_ = (head_ + 1) % N;
    if (count_ < N) ++count_;
  }
  int size() const { return count_; }
  // i = 0 e' l'evento piu' recente.
  const Event& recent(int i) const { return buf_[(head_ - 1 - i + 2 * N) % N]; }

 private:
  Event buf_[N];
  int head_ = 0;
  int count_ = 0;
};
