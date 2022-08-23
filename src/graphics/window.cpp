#include "window.h"
#include "utility.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <cstdlib>
#include <cstdint>

namespace sand {

window::window(const std::string& name, int width, int height)
    : d_data({name, width, height, true, true, true})
{
    if (GLFW_TRUE != glfwInit()) {
		print("FATAL: Failed to initialise GLFW\n");
		std::exit(-1);
	}

	auto native_window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

	if (!native_window) {
		print("FATAL: Failed to create window\n");
		std::exit(-2);
	}

	double x = 0, y = 0;
	glfwGetCursorPos(native_window, &x, &y);
	d_data.mouse_pos = {x, y};

	d_data.native_window = native_window;

	glfwMakeContextCurrent(native_window);
    glfwSetWindowUserPointer(native_window, &d_data);

	// Initialise GLAD
	if (0 == gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		print("FATAL: Failed to initialise GLAD\n");
		std::exit(-3);
	}
    
	int versionMajor;
	int versionMinor;
	glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
	glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
	print("OpenGL version: {}.{}\n", versionMajor, versionMinor);

	// Set OpenGL error callback
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void*) {
		
		switch (severity) {
			case GL_DEBUG_SEVERITY_NOTIFICATION: return;
			case GL_DEBUG_SEVERITY_LOW: {
				print("{}, {}, {}, {}, {}\n", source, type, id, length, message);
			} break;
			case GL_DEBUG_SEVERITY_MEDIUM: {
				print("{}, {}, {}, {}, {}\n", source, type, id, length, message);
			} break;
			case GL_DEBUG_SEVERITY_HIGH: {
				print("{}, {}, {}, {}, {}\n", source, type, id, length, message);
			} break;
		}
	}, nullptr);

	// Set GLFW callbacks
	glfwSetWindowSizeCallback(native_window, [](GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = make_event<window_resize_event>(width, height);
		data->width = width;
		data->height = height;
		data->callback(event);
	});

	glfwSetWindowCloseCallback(native_window, [](GLFWwindow* window)
	{
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = make_event<window_closed_event>();
		data->running = false;
		data->callback(event);
	});

	glfwSetKeyCallback(native_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		switch (action)
		{
			case GLFW_PRESS: {
				auto event = make_event<keyboard_pressed_event>(key, scancode, mods);
				data->callback(event);
			} break;
			case GLFW_RELEASE: {
				auto event = make_event<keyboard_released_event>(key, scancode, mods);
				data->callback(event);
			} break;
			case GLFW_REPEAT: {
				auto event = make_event<keyboard_held_event>(key, scancode, mods);
				data->callback(event);
			} break;
		}
	});

	glfwSetMouseButtonCallback(native_window, [](GLFWwindow* window, int button, int action, int mods) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;

		double x, y;
    	glfwGetCursorPos(data->native_window, &x, &y);

		switch (action)
		{
		case GLFW_PRESS: {
			auto event = make_event<mouse_pressed_event>(
				button, action, mods, glm::vec2{x, y}
			);
			data->callback(event);
		} break;
		case GLFW_RELEASE: {
			auto event = make_event<mouse_released_event>(
				button, action, mods, glm::vec2{x, y}
			);
			data->callback(event);
		} break;
		}
	});

	glfwSetCursorPosCallback(native_window, [](GLFWwindow* window, double x_pos, double y_pos) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = make_event<mouse_moved_event>(
			x_pos, y_pos, x_pos - data->mouse_pos.x, y_pos - data->mouse_pos.y
		);
		data->mouse_pos = {x_pos, y_pos};
		data->callback(event);
	});

	glfwSetScrollCallback(native_window, [](GLFWwindow* window, double x_offset, double y_offset) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = make_event<mouse_scrolled_event>(x_offset, y_offset);
		data->callback(event);
	});

	glfwSetWindowFocusCallback(native_window, [](GLFWwindow* window, int focused) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (focused) {
			auto event = make_event<window_got_focus_event>();
			data->focused = true;
			data->callback(event);
		}
		else {
			auto event = make_event<window_lost_focus_event>();
			data->focused = false;
			data->callback(event);
		}
	});

	glfwSetWindowMaximizeCallback(native_window, [](GLFWwindow* window, int maximized) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (maximized) {
			auto event = make_event<window_maximise_event>();
			data->callback(event);
		}
		else {
			auto event = make_event<window_minimise_event>();
			data->callback(event);
		}
	});

	glfwSetCharCallback(native_window, [](GLFWwindow* window, std::uint32_t key) {
		window_data* data = (window_data*)glfwGetWindowUserPointer(window);
		if (!data->focused) return;
		auto event = make_event<keyboard_typed_event>(key);
		data->callback(event);
	});
}

window::~window()
{
    glfwDestroyWindow(d_data.native_window);
	glfwTerminate();
}

void window::clear() const
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void window::poll_events()
{
    glfwPollEvents();
}

void window::swap_buffers()
{
    glfwSwapBuffers(d_data.native_window);
}

bool window::is_running() const
{
    return d_data.running;
}

glm::vec2 window::get_mouse_pos() const
{
    return d_data.mouse_pos;
}

void window::set_name(const std::string& name)
{
    glfwSetWindowTitle(d_data.native_window, name.c_str());
}

void window::set_callback(const callback_t& callback)
{
    d_data.callback = callback;
}

auto window::width() const -> std::uint32_t
{
	return d_data.width; 
}

auto window::height() const -> std::uint32_t
{
	return d_data.height; 
}

auto window::native_handle() -> GLFWwindow*
{
	return d_data.native_window;
}

}