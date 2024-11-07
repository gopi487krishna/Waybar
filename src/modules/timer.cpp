#include "modules/timer.hpp"

#include <cassert>
#include <string>

#include "gdk/gdk.h"
#include "spdlog/spdlog.h"
#include "util/backend_common.hpp"
#include "util/timer_backend.hpp"

waybar::modules::Timer::Timer(const std::string &id, const Json::Value &config)
    : ALabel(config, "timer", id, "{}%") {
  label_.set_text("00:00:00");
  event_box_.add_events(Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK);
  event_box_.add_events(Gdk::BUTTON_PRESS_MASK);
  event_box_.signal_scroll_event().connect(sigc::mem_fun(*this, &Timer::handleScroll));
  event_box_.signal_button_press_event().connect(sigc::mem_fun(*this, &Timer::handleButtonPress));

  // update callback
  backend = util::TimerBackend::getInstance([this] { this->dp.emit(); });
}

static waybar::util::Segment getCurrentSegment(int total_width, gdouble xpos) {
  double segment_width = (double)total_width / 3;
  int current_segment = xpos / segment_width;
  return (waybar::util::Segment)current_segment;
}

bool waybar::modules::Timer::handleButtonPress(GdkEventButton *e) {
  // Left click : Start/Pause, Right Click : Reset
  if (e->type == GDK_BUTTON_PRESS) {
    const int LEFT_BUTTON_PRESSED = 1;
    const int RIGHT_BUTTON_PRESSED = 3;
    switch (e->button) {
      case LEFT_BUTTON_PRESSED:
        backend->toggleTimer();
        break;
      case RIGHT_BUTTON_PRESSED:
        backend->resetTimer();
        break;
      default:;
    }
  } else if (e->type == GDK_DOUBLE_BUTTON_PRESS) {
    /*
     * A user can double press on a perticular segment
     * to increment the value of that segment by some
     * fixed value
     */
    int event_box_width = event_box_.get_allocated_width();
    auto current_segment = getCurrentSegment(event_box_width, e->x);
    auto increase = util::ChangeType::Increase;
    switch (current_segment) {
      case waybar::util::Segment::HOUR:
        backend->updateDuration(current_segment, increase, 1);
        break;
      case waybar::util::Segment::MINUTE:
        backend->updateDuration(current_segment, increase, 15);
        break;
      case waybar::util::Segment::SECOND:
        backend->updateDuration(current_segment, increase, 30);
        break;
      default:
        spdlog::error("Timer : Invalid segment");
    }
  }

  return true;
}

bool waybar::modules::Timer::handleScroll(GdkEventScroll *e) {
  auto dir = AModule::getScrollDir(e);
  if (dir == SCROLL_DIR::NONE) {
    return true;
  }

  /* Do not handle scroll events when the timer is running */
  if (backend->isActive()) return true;

  int event_box_width = event_box_.get_allocated_width();
  auto current_segment = getCurrentSegment(event_box_width, e->x);
  util::ChangeType changetype;
  if (dir == SCROLL_DIR::UP)
    changetype = util::ChangeType::Increase;
  else
    changetype = util::ChangeType::Decrease;

  backend->updateDuration(current_segment, changetype, 1);

  return true;
}

auto waybar::modules::Timer::update() -> void {
  const std::string current_value = backend->getCurrentValue();
  const std::string expiring_class = "expiring";
  label_.set_markup(current_value);

  auto classes = label_.get_style_context()->list_classes();

  auto expiring_class_it = std::find(classes.begin(), classes.end(), expiring_class);
  bool expiring = backend->isExpiring();

  if (expiring) {
    if (expiring_class_it == classes.end()) label_.get_style_context()->add_class(expiring_class);
  } else if (expiring_class_it != classes.end()) {
    label_.get_style_context()->remove_class(expiring_class);
  }

  // Call parent update
  ALabel::update();
}
