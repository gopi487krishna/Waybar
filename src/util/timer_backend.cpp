#include "util/timer_backend.hpp"

#include <fcntl.h>
#include <sys/mman.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <string>

#include "util/backend_common.hpp"

namespace waybar::util {

class TimerImpl {
 public:
  void start(std::uint64_t duration_s);
  void pause();
  void resume();
  void reset();
  std::uint64_t getDuration() const;
  bool isRunning() const { return is_running_; }
  bool isActive() const { return duration_.load(); }
  std::uint64_t readRemainingSeconds();
  std::chrono::high_resolution_clock::time_point start_val_;
  std::chrono::high_resolution_clock::time_point pause_start_;

 private:
  std::atomic_uint64_t duration_ = 0;
  std::atomic_bool is_running_ = false;
  std::uint64_t readSecondsElapsed() const;
};

void TimerImpl::start(std::uint64_t duration_s) {
  start_val_ = std::chrono::high_resolution_clock::now();
  duration_.store(duration_s);
  is_running_.store(true);
}

void TimerImpl::pause() {
  if (!is_running_.load()) return;
  is_running_.store(false);
  pause_start_ = std::chrono::high_resolution_clock::now();
}

void TimerImpl::resume() {
  if (is_running_.load()) return;

  auto pause_end = std::chrono::high_resolution_clock::now();
  auto pause_duration = pause_end - pause_start_;
  start_val_ += pause_duration;
  is_running_.store(true);
}

void TimerImpl::reset() {
  is_running_.store(false);
  duration_.store(0);
  start_val_ = {};
  pause_start_ = {};
}

std::uint64_t TimerImpl::readRemainingSeconds() {
  uint64_t duration_old;
  uint64_t duration_new;
  uint64_t seconds_elapsed;
  do {
    seconds_elapsed = readSecondsElapsed();
    duration_old = duration_.load();
    duration_new = duration_old < seconds_elapsed ? 0 : duration_old;
  } while (!duration_.compare_exchange_strong(duration_old, duration_new));

  /* If timer is completed, reset the state of timer */
  if (!duration_new) {
    reset();
  }

  return duration_new ? duration_new - seconds_elapsed : 0;
}

std::uint64_t TimerImpl::readSecondsElapsed() const {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now - start_val_).count();
}

std::uint64_t TimerImpl::getDuration() const { return duration_; }

TimerBackend::TimerBackend(std::function<void()> on_updated_cb, private_constructor_tag tag)
    : timer_impl_(new TimerImpl()), on_updated_cb_(std::move(on_updated_cb)) {
  shutdown_.store(false);
  thread_ = [this] { this->timerWorker(); };
}

void TimerBackend::timerWorker() {
  while (!shutdown_.load()) {
    if (timer_impl_->isRunning()) updateTimer(timer_impl_->readRemainingSeconds());
    thread_.sleep_for(std::chrono::seconds(1));
  }
}

void TimerBackend::resetHMS() {
  hms_.hour_ = 0;
  hms_.minute_ = 0;
  hms_.second_ = 0;
}
void TimerBackend::resetTimer() {
  timer_impl_->reset();
  resetHMS();
  on_updated_cb_();
}

void TimerBackend::toggleTimer() {
  if (timer_impl_->isRunning())
    timer_impl_->pause();
  else if (!timer_impl_->isActive())
    timer_impl_->start(convertHMSToSeconds());
  else
    timer_impl_->resume();
}

std::uint64_t TimerBackend::convertHMSToSeconds() const {
  const uint64_t hour_seconds = hms_.hour_ * (uint64_t)(3600);
  const uint64_t min_seconds = hms_.minute_ * (uint64_t)(60);
  const uint64_t total_seconds = hour_seconds + min_seconds + hms_.second_;
  return total_seconds;
}
void TimerBackend::updateHMS(uint64_t seconds_elapsed) {
  hms_.hour_ = seconds_elapsed / 3600;
  hms_.minute_ = (seconds_elapsed % 3600) / 60;
  hms_.second_ = (seconds_elapsed % 60);
}

void TimerBackend::updateTimer(uint64_t seconds_elapsed) {
  updateHMS(seconds_elapsed);
  on_updated_cb_();
}

TimerBackend::~TimerBackend() { shutdown_.store(true); }

std::shared_ptr<TimerBackend> TimerBackend::getInstance(std::function<void()> on_updated_cb) {
  private_constructor_tag tag;
  return std::make_shared<TimerBackend>(on_updated_cb, tag);
}

static std::string formatSegment(std::uint8_t val) {
  std::string val_str = std::to_string(val);
  return val < 10 ? "0" + val_str : val_str;
}

bool TimerBackend::isActive() const { return timer_impl_->isActive(); }
bool TimerBackend::isExpiring() const {
  if (timer_impl_->getDuration()) return timer_impl_->readRemainingSeconds() < 10;

  return false;
}
std::string TimerBackend::getCurrentValue() const {
  return formatSegment(hms_.hour_) + ":" + formatSegment(hms_.minute_) + ":" +
         formatSegment(hms_.second_);
}

void TimerBackend::changeSegmentValue(Segment current_segment, int step) {
  switch (current_segment) {
    case Segment::HOUR:
      hms_.hour_ = (hms_.hour_ + step + 23) % 23;
      break;
    case Segment::MINUTE:
      hms_.minute_ = (hms_.minute_ + step + 60) % 60;
      break;
    case Segment::SECOND:
      hms_.second_ = (hms_.second_ + step + 60) % 60;
      break;
    default:
      assert(false && "Unable to handle current_segment");
  }
}

void TimerBackend::updateDuration(Segment current_segment, util::ChangeType change_type, int step) {
  switch (change_type) {
    case util::ChangeType::Increase:
      changeSegmentValue(current_segment, step);
      break;
    case util::ChangeType::Decrease:
      changeSegmentValue(current_segment, -1 * step);
      break;
    default:
      assert(false && "Unable to handle change_type");
  }
  on_updated_cb_();
}

}  // namespace waybar::util
