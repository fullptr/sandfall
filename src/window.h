#ifndef INCLUDED_ALCHIMIA_WINDOW
#define INCLUDED_ALCHIMIA_WINDOW
#include "event.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <cstdint>
#include <functional>

struct GLFWwindow;

namespace alc {

using callback_t = std::function<void(alc::event&)>;

struct window_data
{
    std::string name;

    int width;
    int height;

    bool fullscreen;
    bool running;
    bool focused;

    GLFWwindow* native_window;

    callback_t callback = [](event&) {};
};

class window
{
    window_data d_data;

public:
    window(const std::string&, int width, int height);
    ~window();

    void clear() const;
    void swap_and_poll();

    bool is_running() const;

    glm::vec2 get_mouse_pos() const;

    void set_name(const std::string& name);
    void set_callback(const callback_t& callback);

    float width() const { return (float)d_data.width; }
    float height() const { return (float)d_data.height; }

private:
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    window(window&&) = delete;
    window& operator=(window&&) = delete;
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW