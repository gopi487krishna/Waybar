#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <memory>

#include "ALabel.hpp"
#include "util/timer_backend.hpp"

namespace waybar::modules {

class Timer : public ALabel {
 public:
  Timer(const std::string&, const Json::Value&);
  virtual ~Timer() = default;
  auto update() -> void override;

 private:
  bool handleScroll(GdkEventScroll* e) override;
  bool handleButtonPress(GdkEventButton* e);
  std::shared_ptr<util::TimerBackend> backend;
};

}  // namespace waybar::modules
