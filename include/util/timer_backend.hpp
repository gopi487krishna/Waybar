#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "util/backend_common.hpp"
#include "util/sleeper_thread.hpp"

namespace waybar::util {

class TimerImpl;

enum class Segment { HOUR, MINUTE, SECOND };

class TimerBackend {
  struct HMS {
    std::uint8_t hour_ = 0;
    std::uint8_t minute_ = 0;
    std::uint8_t second_ = 0;
  };

  std::unique_ptr<TimerImpl> timer_impl_;

  /* Hack to keep constructor inaccessible but still public.
   * This is required to be able to use std::make_shared.
   * It is important to keep this class only accessible via a reference-counted
   * pointer because the destructor will manually free memory, and this could be
   * a problem with C++20's copy and move semantics.
   */
  struct private_constructor_tag {};
  util::SleeperThread thread_;
  std::atomic_bool shutdown_;
  std::function<void()> on_updated_cb_ = NOOP;
  HMS hms_;

 public:
  TimerBackend(std::function<void()> on_updated_cb, private_constructor_tag tag);
  ~TimerBackend();
  static std::shared_ptr<TimerBackend> getInstance(std::function<void()> on_updated_cb = NOOP);
  void updateDuration(Segment current_segment, util::ChangeType change_type, int step);
  void updateTimer(uint64_t seconds);
  void toggleTimer();
  void resetTimer();
  void timerWorker();
  std::string getCurrentValue() const;
  bool isExpiring() const;
  bool isActive() const;

 private:
  void changeSegmentValue(Segment current_segment, int step);
  void updateHMS(uint64_t seconds);
  void resetHMS();
  std::uint64_t convertHMSToSeconds() const;
};

}  // namespace waybar::util
