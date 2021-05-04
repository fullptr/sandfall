#include <fmt/format.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main()
{
    // Set up window
    if (GLFW_TRUE != glfwInit()) {
        fmt::print("Failed to initialise GLFW\n");
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Alchimia", nullptr, nullptr);
    if (!window) {
        fmt::print("Failed to create window\n");
        return -2;
    }

    glfwMakeContextCurrent(window);

    if (0 == gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fmt::print("Failed to initialise GLAD\n");
        return -3;
    }

    // Set callbacks here

    // Main game loop
    while (true) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	    glClearColor(0.0, 0.0, 0.0, 1.0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        glfwSetWindowTitle(window, fmt::format("Mouse at ({}, {})", x, y).c_str());
    }
}