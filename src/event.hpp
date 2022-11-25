#pragma once
#include <glm/glm.hpp>

#include <any>
#include <variant>
#include <string>
#include <cstdint>

namespace sand {

// KEYBOARD EVENTS 
struct keyboard_pressed_event {
	int key;
	int scancode;
	int mods;
};

struct keyboard_released_event {
	int key;
	int scancode;
	int mods;
};

struct keyboard_held_event {
	int key;
	int scancode;
	int mods;
};

struct keyboard_typed_event {
	std::uint32_t key;
};

// MOUSE EVENTS
struct mouse_pressed_event {
	int button;
	int action;
	int mods;
	glm::vec2 pos;
};

struct mouse_released_event {
	int button;
	int action;
	int mods;
	glm::vec2 pos;
};

struct mouse_moved_event {
	glm::vec2 pos;
	glm::vec2 offset;
};

struct mouse_scrolled_event {
	glm::vec2 offset;
};

// WINDOW EVENTS
struct window_resize_event {
	int width;
	int height;
};

struct window_closed_event {};
struct window_got_focus_event {};
struct window_lost_focus_event {};
struct window_maximise_event {};
struct window_minimise_event {};

class event
{
	using event_variant = std::variant<
		keyboard_pressed_event,
		keyboard_released_event,
		keyboard_held_event,
		keyboard_typed_event,
		mouse_pressed_event,
		mouse_released_event,
		mouse_moved_event,
		mouse_scrolled_event,
		window_resize_event,
		window_closed_event,
		window_got_focus_event,
		window_lost_focus_event,
		window_maximise_event,
		window_minimise_event
	>;

    event_variant d_event;

public:
	template <typename T, typename... Args>
	explicit event(std::in_place_type_t<T>, Args&&... args)
		: d_event(std::in_place_type<T>, std::forward<Args>(args)...)
	{}

	template <typename T>
	auto is() const noexcept -> bool { return std::holds_alternative<T>(d_event); }

	template <typename T>
	auto as() const -> const T& { return std::get<T>(d_event); }

	template <typename Visitor>
	auto visit(Visitor&& visitor) const { return std::visit(std::forward<Visitor>(visitor), d_event); }

	auto is_keyboard_event() const -> bool;
	auto is_mount_event() const -> bool;
	auto is_window_event() const -> bool;
};

template <typename T, typename... Args>
event make_event(Args&&... args)
{
	return event(std::in_place_type<T>, std::forward<Args>(args)...);
}

inline auto event::is_keyboard_event() const -> bool
{
	return is<keyboard_held_event>()
		|| is<keyboard_typed_event>()
		|| is<keyboard_released_event>()
		|| is<keyboard_typed_event>();
}

inline auto event::is_mount_event() const -> bool
{
	return is<mouse_moved_event>()
		|| is<mouse_pressed_event>()
		|| is<mouse_released_event>()
		|| is<mouse_scrolled_event>();
}

inline auto event::is_window_event() const -> bool
{
	return is<window_resize_event>()
		|| is<window_closed_event>()
		|| is<window_got_focus_event>()
		|| is<window_lost_focus_event>()
		|| is<window_maximise_event>()
		|| is<window_minimise_event>();
}

}