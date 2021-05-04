#ifndef INCLUDED_ALCHIMIA_EVENT
#define INCLUDED_ALCHIMIA_EVENT
#include <any>
#include <string>
#include <cstdint>

namespace alc {

class event
{
    std::any d_event;
    bool     d_consumed;

public:
	template <typename T, typename... Args>
	explicit event(std::in_place_type_t<T>, Args&&... args)
		: d_event(std::in_place_type<T>, std::forward<Args>(args)...)
        , d_consumed(false)
	{}

	template <typename T> bool is() const noexcept { return get_if<T>() != nullptr; }
	template <typename T> const T& get() const { return std::any_cast<const T&>(d_event); }
	template <typename T> const T* get_if() const noexcept { return std::any_cast<T>(&d_event); }

	bool is_consumed() const noexcept { return d_consumed; }
	void consume() noexcept { d_consumed = true; }

	// Implementation defined name, should only be used for logging.
	std::string type_name() const noexcept { return d_event.type().name(); }

	const std::type_info& type_info() const noexcept { return d_event.type(); }
};

template <typename T, typename... Args>
event make_event(Args&&... args)
{
	return event(std::in_place_type<T>, std::forward<Args>(args)...);
}

// KEYBOARD EVENTS 
struct keyboard_pressed_event {
	int key;
	int scancode;
	int mods;
	keyboard_pressed_event(int k, int s, int m) : key(k), scancode(s), mods(m) {}
};

struct keyboard_released_event {
	int key;
	int scancode;
	int mods;
	keyboard_released_event(int k, int s, int m) : key(k), scancode(s), mods(m) {}
};

struct keyboard_held_event {
	int key;
	int scancode;
	int mods;
	keyboard_held_event(int k, int s, int m) : key(k), scancode(s), mods(m) {}
};

struct keyboard_typed_event {
	std::uint32_t key;
	keyboard_typed_event(std::uint32_t k) : key(k) {}
};

// MOUSE EVENTS
struct mouse_pressed_event {
	int button;
	int action;
	int mods;
	mouse_pressed_event(int b, int a, int m) : button(b), action(a), mods(m) {}
};

struct mouse_released_event {
	int button;
	int action;
	int mods;
	mouse_released_event(int b, int a, int m) : button(b), action(a), mods(m) {}
};

struct mouse_moved_event {
	float x_pos;
	float y_pos;
	mouse_moved_event(float x, float y) : x_pos(x), y_pos(y) {}
};

struct mouse_scrolled_event {
	float x_offset;
	float y_offset;
	mouse_scrolled_event(float x, float y) : x_offset(x), y_offset(y) {}
};

// WINDOW EVENTS
struct window_resize_event {
	int width;
	int height;
	window_resize_event(int w, int h) : width(w), height(h) {}
};

struct window_closed_event {};
struct window_got_focus_event {};
struct window_lost_focus_event {};
struct window_maximise_event {};
struct window_minimise_event {};

}

#endif // INCLUDED_ALCHIMIA_EVENT