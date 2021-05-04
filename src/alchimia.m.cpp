#include "window.h"
#include "log.h"
#include "shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <cstddef>

struct vertex
{
    glm::vec2 pos;
};

int main()
{
    using namespace alc;

    alc::window window("alchimia", 1280, 720);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //const char *vertexShaderSource = "#version 330 core\n"
    //"layout (location = 0) in vec3 aPos;\n"
    //"void main()\n"
    //"{\n"
    //"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    //"}\0";
    //unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    //glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    //glCompileShader(vertexShader);
//
    //const char* fragmentShaderSource = "#version 330 core"
    //"out vec4 FragColor;"
    //""
    //"void main()"
    //"{"
    //"    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);"
    //"}";
    //unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    //glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    //glCompileShader(fragmentShader);
//
    //unsigned int shaderProgram = glCreateProgram();
    //glAttachShader(shaderProgram, vertexShader);
    //glAttachShader(shaderProgram, fragmentShader);
    //glLinkProgram(shaderProgram);

    alc::shader shader("res\\vertex.glsl", "res\\fragment.glsl");

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.bind();
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    while (window.is_running()) {
        window.clear();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        window.swap_and_poll();
        
        auto mouse = window.get_mouse_pos();
        window.set_name(fmt::format("Mouse at ({}, {})", mouse.x, mouse.y));
    }
}