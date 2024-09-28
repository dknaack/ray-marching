#include <assert.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "glad/glad.c"

#define FLOAT_PI 3.14159265358979323846f

typedef union {
	struct { float x, y; };
	float e[2];
} vec2;

typedef union {
	struct { float x, y, z; };
	float e[3];
} vec3;

typedef struct {
	float e[4][4];
} mat4;

typedef struct {
	char *at;
	size_t length;
} string;

static float
radians(float degrees)
{
	float result = FLOAT_PI / 180.0f * degrees;
	return result;
}

static vec2
make_vec2(float x, float y)
{
	vec2 result;
	result.x = x;
	result.y = y;
	return result;
}

static vec3
make_vec3(float x, float y, float z)
{
	vec3 result;
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

static vec3
add3(vec3 a, vec3 b)
{
	vec3 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

static vec3
sub3(vec3 a, vec3 b)
{
	vec3 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

static vec3
mulf3(vec3 a, float b)
{
	vec3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	return result;
}

static float
dot3(vec3 a, vec3 b)
{
	float result = a.x * b.x + a.y * b.y + a.z * b.z;
	return result;
}

static vec3
normalize3(vec3 a)
{
	vec3 result;
	float length = sqrtf(dot3(a, a));
	result.x = a.x / length;
	result.y = a.y / length;
	result.z = a.z / length;
	return result;
}

static vec3
cross(vec3 a, vec3 b)
{
	vec3 result;
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	return result;
}

// NOTE: This transform a vector in world space to view space.
static mat4
look_at(vec3 eye, vec3 center, vec3 up)
{
	vec3 f = normalize3(sub3(center, eye));
	vec3 s = normalize3(cross(f, up));
	vec3 u = cross(s, f);

	mat4 result = {0};
	result.e[0][0] = s.x;
	result.e[0][1] = s.y;
	result.e[0][2] = s.z;

	result.e[1][0] = u.x;
	result.e[1][1] = u.y;
	result.e[1][2] = u.z;

	result.e[2][0] = -f.x;
	result.e[2][1] = -f.y;
	result.e[2][2] = -f.z;

	result.e[3][0] = eye.x;
	result.e[3][1] = eye.y;
	result.e[3][2] = eye.z;
	result.e[3][3] = 1.0f;
	return result;
}

static string
read_file(char *filename)
{
	string result = {0};
	FILE *file = fopen(filename, "r");
	if (file) {
		fseek(file, 0, SEEK_END);
		result.length = ftell(file);
		fseek(file, 0, SEEK_SET);

		result.at = calloc(result.length + 1, 1);
		result.at[result.length] = 0;
		fread(result.at, 1, result.length, file);
	}

	return result;
}

static GLuint
gl_shader_create(char *src, GLenum type)
{
	GLuint shader = glCreateShader(type);

	const char *strings[1];
	strings[0] = src;

	glShaderSource(shader, 1, strings, NULL);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetShaderInfoLog(shader, sizeof(info), NULL, info);
		fprintf(stderr, "gl_shader_create: %s\n", info);
	}

	return shader;
}

static GLuint
gl_program_create(char *vs, char *fs)
{
	GLuint vertex_shader = gl_shader_create(vs, GL_VERTEX_SHADER);
	if (!vertex_shader) {
		assert(!"Invalid vertex shader");
		return 0;
	}

	GLuint fragment_shader = gl_shader_create(fs, GL_FRAGMENT_SHADER);
	if (!fragment_shader) {
		glDeleteShader(vertex_shader);
		assert(!"Invalid fragment shader");
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetProgramInfoLog(program, sizeof(info), 0, info);
		fprintf(stderr, "gl_program_create: %s\n", info);
	}

	return program;
}

static GLuint program;

static void
gl_uniform_vec2(GLint program, char *name, vec2 value)
{
	GLint location = glGetUniformLocation(program, name);
	glUniform2f(location, value.x, value.y);
}

static void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	gl_uniform_vec2(program, "size", make_vec2(width, height));
}

static vec2
get_mouse_pos(GLFWwindow *window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	vec2 result = make_vec2(x, y);
	return result;
}

int main(void)
{
	if (!glfwInit()) {
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	char *vs_src =
		"#version 330 core\n"
		"layout (location = 0) in vec3 pos;\n"
		"void main(void) {\n"
		"	gl_Position = vec4(pos, 1);\n"
		"}\n";
	string fs_src = read_file("main.glsl");
	program = gl_program_create(vs_src, fs_src.at);
	glUseProgram(program);
	free(fs_src.at);

	float vertices[] = {
		// first triangle
		1.0f,  1.0f, 0.0f,  // top right
		1.0f, -1.0f, 0.0f,  // bottom right
		-1.0f,  1.0f, 0.0f,  // top left
							 // second triangle
		1.0f, -1.0f, 0.0f,  // bottom right
		-1.0f, -1.0f, 0.0f,  // bottom left
		-1.0f,  1.0f, 0.0f   // top left
	};

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	vec3 camera_pos = make_vec3(0, 0, 5);
	vec2 prev_mouse_pos = get_mouse_pos(window);
	float pitch = 0;
	float yaw = 0;

	/* Loop until the user closes the window */
	float prev_time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		float time = glfwGetTime();
		float dt = time - prev_time;

		// Update the camera
		vec2 mouse_pos = get_mouse_pos(window);
		float sensitivity = 0.4f;
		yaw   += (mouse_pos.x - prev_mouse_pos.x) * sensitivity;
		pitch -= (mouse_pos.y - prev_mouse_pos.y) * sensitivity;
		if (pitch < -89.0f) {
			pitch = -89.0f;
		} else if (pitch > 10.1f) {
			pitch = 10.1f;
		}

		// Get the camera direction
		vec3 camera_dir;
		camera_dir.x = cos(radians(yaw)) * cos(radians(pitch));
		camera_dir.y = sin(radians(pitch));
		camera_dir.z = sin(radians(yaw)) * cos(radians(pitch));
		camera_dir = normalize3(camera_dir);

		// Update the camera position
		float speed = 10.0f * dt;
		vec3 camera_right = cross(make_vec3(0, 1, 0), camera_dir);
		if (glfwGetKey(window, GLFW_KEY_W)) {
			camera_pos = add3(camera_pos, mulf3(camera_dir, speed));
		}

		if (glfwGetKey(window, GLFW_KEY_A)) {
			camera_pos = add3(camera_pos, mulf3(camera_right, speed));
		}

		if (glfwGetKey(window, GLFW_KEY_S)) {
			camera_pos = sub3(camera_pos, mulf3(camera_dir, speed));
		}

		if (glfwGetKey(window, GLFW_KEY_D)) {
			camera_pos = sub3(camera_pos, mulf3(camera_right, speed));
		}

		// Get the view matrix
		mat4 view = look_at(camera_pos, add3(camera_pos, camera_dir), make_vec3(0, 1, 0));
		glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, (float *)view.e);

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glUniform1f(glGetUniformLocation(program, "time"), time);

		glfwSwapBuffers(window);
		glfwPollEvents();
		prev_mouse_pos = mouse_pos;
		prev_time = time;
	}

	glfwTerminate();
	return 0;
}
