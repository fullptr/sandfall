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

    callback_t callback;
};

class window
{
    GLFWwindow* d_native_window;

    window_data d_data;

public:
    window(const std::string&, int width, int height);
    ~window();

    void on_update(double dt);

    glm::vec2 get_mouse_pos() const;

    void set_name(const std::string& name);
    void set_callback(const callback_t& callback);

private:
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    window(window&&) = delete;
    window& operator=(window&&) = delete;
};

}

#endif // INCLUDED_ALCHIMIA_WINDOW