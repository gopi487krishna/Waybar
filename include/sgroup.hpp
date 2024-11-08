#pragma once

#include <gtkmm/box.h>
#include <gtkmm/widget.h>
#include <json/json.h>
#include <group.hpp>
#include "gdk/gdk.h"

namespace waybar {

class SGroup : public Group {
 public:
  SGroup(const std::string &, const std::string &, const Json::Value &, bool);

  ~SGroup() override = default;
  auto update() -> void override;
  operator Gtk::Widget &() override;

  Gtk::Box &getBox() override;
  void addWidget(Gtk::Widget &widget) override;

 protected:
  Gtk::Stack stack;
  bool handleMouseEnter(GdkEventCrossing *const &ev) override;
  bool handleMouseLeave(GdkEventCrossing *const &ev) override;
  bool handleToggle(GdkEventButton *const &ev) override;
  bool handleScroll(GdkEventScroll* e) override;
  bool handleScrollx(GdkEventScroll* e);
  virtual bool handleKeyPress(GdkEventKey *const &ev);
};

}
