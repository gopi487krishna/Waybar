#include "sgroup.hpp"

#include <fmt/format.h>

#include <util/command.hpp>

#include "group.hpp"
#include "gtkmm/enums.h"
#include "gtkmm/widget.h"
#include "gdk/gdkkeysyms.h"

namespace waybar {

SGroup::SGroup(const std::string& name, const std::string& id, const Json::Value& config,
             bool vertical)
    : Group(name, id, config, vertical) {

  event_box_.signal_scroll_event().connect(sigc::mem_fun(*this, &SGroup::handleScroll));
  // Revealer and all other widgets are useless for us
  for (auto* widget : box.get_children()) {
    box.remove(*widget);
  }
  box.pack_start(stack);
}


bool SGroup::handleMouseEnter(GdkEventCrossing* const& e) {
  box.set_state_flags(Gtk::StateFlags::STATE_FLAG_PRELIGHT);
  auto num_widgets = stack.get_children().size();
  auto visible_child_id = std::atoi(stack.get_visible_child_name().c_str());
  visible_child_id = (visible_child_id + 1) % num_widgets;
  stack.set_visible_child(std::to_string(visible_child_id));
  return true;
}


bool SGroup::handleScroll(GdkEventScroll* e) {
  box.set_state_flags(Gtk::StateFlags::STATE_FLAG_PRELIGHT);

  return true;
}

bool SGroup::handleMouseLeave(GdkEventCrossing* const& e) {
  box.set_state_flags(Gtk::StateFlags::STATE_FLAG_PRELIGHT);
  return true;
}

bool SGroup::handleToggle(GdkEventButton* const& e) {
  box.set_state_flags(Gtk::StateFlags::STATE_FLAG_PRELIGHT);
  return true;
}

bool SGroup::handleKeyPress(GdkEventKey *const &ev) {

  return true;
}

auto SGroup::update() -> void {
  // noop
}

Gtk::Box& SGroup::getBox() { return box; }

void SGroup::addWidget(Gtk::Widget& widget) {
  auto widget_id = stack.get_children().size();
  stack.add(widget, std::to_string(widget_id));
}

SGroup::operator Gtk::Widget&() { return event_box_; }

}
