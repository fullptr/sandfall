#include "window.h"
#include "log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <cstdlib>

namespace alc {

window::window(const std::string& name, int width, int height)
    : d_native_window(nullptr)
    , d_data({name, width, height, true, true, true})
{
    if (GLFW_TRUE != glfwInit()) {
		log::fatal("Failed to initialise GLFW\n");
		std::exit(-1);
	}

	d_native_window = glfwCreateWindow(
		width, height, name.c_str(), nullptr, nullptr
	);

	if (!d_native_window) {
		log::fatal("Failed to create window\n");
		std::exit(-2);
	}

	glfwMakeContextCurrent(d_native_window);
    glfwSetWindowUserPointer(d_native_window, &d_data);

	// Initialise GLAD
	if (0 == gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		log::fatal("Failed to initialise GLAD\n");
		std::exit(-3);
	}
}

window::~window()
{
    glfwDestroyWindow(d_native_window);
	glfwTerminate();
}

void window::on_update(double dt)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    glfwSwapBuffers(d_native_window);
    glfwPollEvents();
}

glm::vec2 window::get_mouse_pos() const
{
    double x, y;
    glfwGetCursorPos(d_native_window, &x, &y);
    return {x, y};
}

void window::set_name(const std::string& name)
{
    glfwSetWindowTitle(d_native_window, name.c_str());
}

void window::set_callback(const callback_t& callback)
{
    d_data.callback = callback;
}

}