#include "shader.h"
#include "log.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <filesystem>
#include <fstream>

namespace alc {
namespace {

std::uint32_t compile_shader(std::uint32_t type, const std::string& source)
{
	std::uint32_t id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE) {
        log::error("Could not compile shader {}\n", (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");
		glDeleteShader(id);
		return 0;
	}

	return id;
}

}

std::string parse_shader(const std::string& filepath)
{
	if (!std::filesystem::exists(filepath)) {
		log::fatal("Shader file '{}' does not exist!", filepath);
	}
	std::ifstream stream(filepath);
	std::string shader((std::istreambuf_iterator<char>(stream)),
		                std::istreambuf_iterator<char>());
	return shader;
}

shader::shader(const std::string& vertex_shader, const std::string& fragment_shader)
    : d_program(glCreateProgram())
    , d_vertex_shader(compile_shader(GL_VERTEX_SHADER, parse_shader(vertex_shader)))
    , d_fragment_shader(compile_shader(GL_FRAGMENT_SHADER, parse_shader(fragment_shader)))
{
    glAttachShader(d_program, d_vertex_shader);
	glAttachShader(d_program, d_fragment_shader);
	glLinkProgram(d_program);
	glValidateProgram(d_program);
}

std::uint32_t shader::get_location(const std::string& name) const
{
    return glGetUniformLocation(d_program, name.c_str());
}

void shader::bind() const
{
    glUseProgram(d_program);
}

void shader::unbind() const
{
    glUseProgram(0);
}

void shader::load_mat4(const std::string& name, const glm::mat4& matrix) const
{
    glUniformMatrix4fv(get_location(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void shader::load_sampler(const std::string& name, int value) const
{
	glProgramUniform1i(d_program, get_location(name), value);
}

}