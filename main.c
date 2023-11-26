#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "glad/glad.c"

#define ASSERT(expr) ((expr) ? (void)0 : (void)(*(volatile int *)0 = 0))
#define ALLOC(arena, count, type) ((type *)alloc(arena, count, sizeof(type)))

#define MAX_POINT_COUNT 1024
#define F32_PI 3.14159265358979323846f

typedef uintptr_t usize;
typedef uint64_t  u64;
typedef uint32_t  u32;
typedef uint16_t  u16;
typedef uint8_t   u8;

typedef intptr_t isize;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

typedef double f64;
typedef float  f32;
typedef i32 b32;

typedef union {
	struct { f32 x, y; };
	f32 e[2];
} v2;

typedef union {
    struct { f32 x, y, z; };
    f32 e[3];
} v3;

typedef struct {
    f32 e[4][4];
} m4x4;

typedef struct {
    char *at;
    usize length;
} string;

typedef struct {
	char *data;
	usize pos;
	usize size;
} arena;

static f32
radians(f32 degrees)
{
	f32 result = F32_PI / 180.0f * degrees;
	return result;
}

static v2
make_v2(f32 x, f32 y)
{
	v2 result;
	result.x = x;
	result.y = y;
	return result;
}

static v3
make_v3(f32 x, f32 y, f32 z)
{
    v3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static v3
add3(v3 a, v3 b)
{
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static v3
sub3(v3 a, v3 b)
{
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static v3
mulf3(v3 a, f32 b)
{
    v3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

static f32
dot3(v3 a, v3 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

static v3
normalize3(v3 a)
{
    v3 result;
    f32 length = sqrtf(dot3(a, a));
    result.x = a.x / length;
    result.y = a.y / length;
    result.z = a.z / length;
    return result;
}

static v3
cross(v3 a, v3 b)
{
    v3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

// NOTE: This transform a vector in world space to view space.
static m4x4
look_at(v3 eye, v3 center, v3 up)
{
	v3 f = normalize3(sub3(center, eye));
	v3 s = normalize3(cross(f, up));
	v3 u = cross(s, f);

	m4x4 result = {0};
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

static arena *
new_arena(size_t size)
{
	arena *arena = calloc(size + sizeof(*arena), 1);
	arena->data = (char *)(arena + 1);
	arena->size = size;
	return arena;
}

static void *
alloc(arena *arena, size_t size, size_t count)
{
	ASSERT(arena->pos < arena->size);
	void *result = arena->data + arena->pos;
	arena->pos += size * count;
	return result;
}

static string
read_file(char *filename, arena *arena)
{
    string result = {0};
    FILE *file = fopen(filename, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        result.length = ftell(file);
        fseek(file, 0, SEEK_SET);

        result.at = ALLOC(arena, result.length + 1, char);
        result.at[result.length] = 0;
        fread(result.at, 1, result.length, file);
    }

    return result;
}

static GLuint
gl_shader_create(char *src, GLenum type)
{
	GLuint shader = glCreateShader(type);

	const char *strings[3];
	strings[0] = "#version 330\n";
	strings[1] =
		"#define v2 vec2\n"
		"#define v3 vec3\n"
		"#define v4 vec4\n"
		"#define m4x4 mat4\n"
		"#define f32 float\n"
		"#define u32 uint\n"
		"#define i32 int\n";
	strings[2] = src;

	glShaderSource(shader, 3, strings, NULL);
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
		ASSERT(!"Invalid vertex shader");
		return 0;
	}

	GLuint fragment_shader = gl_shader_create(fs, GL_FRAGMENT_SHADER);
	if (!fragment_shader) {
		glDeleteShader(vertex_shader);
		ASSERT(!"Invalid fragment shader");
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
gl_uniform_v2(GLint program, char *name, v2 value)
{
    GLint location = glGetUniformLocation(program, name);
    glUniform2f(location, value.x, value.y);
}

static void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    gl_uniform_v2(program, "size", make_v2(width, height));
}

static v2
get_mouse_pos(GLFWwindow *window)
{
    f64 x, y;
    glfwGetCursorPos(window, &x, &y);

    v2 result = make_v2(x, y);
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

	arena *arena = new_arena(1024 * 1024);
	char *vs_src =
		"layout (location = 0) in v3 pos;\n"
		"void main(void) {\n"
		"	gl_Position = v4(pos, 1);\n"
		"}\n";
	string fs_src = read_file("main.glsl", arena);
	program = gl_program_create(vs_src, fs_src.at);
	glUseProgram(program);

	f32 vertices[] = {
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

    v3 camera_pos = make_v3(0, 0, 5);
	v2 prev_mouse_pos = get_mouse_pos(window);
	f32 pitch = 0;
	f32 yaw = 0;

    /* Loop until the user closes the window */
    f32 prev_time = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        f32 time = glfwGetTime();
        f32 dt = time - prev_time;

        // Update the camera
        v2 mouse_pos = get_mouse_pos(window);
        f32 sensitivity = 0.4f;
        yaw   += (mouse_pos.x - prev_mouse_pos.x) * sensitivity;
        pitch -= (mouse_pos.y - prev_mouse_pos.y) * sensitivity;
        if (pitch < -89.0f) {
            pitch = -89.0f;
        } else if (pitch > 10.1f) {
            pitch = 10.1f;
        }

		// Get the camera direction
        v3 camera_dir;
        camera_dir.x = cos(radians(yaw)) * cos(radians(pitch));
        camera_dir.y = sin(radians(pitch));
        camera_dir.z = sin(radians(yaw)) * cos(radians(pitch));
        camera_dir = normalize3(camera_dir);

        // Update the camera position
        f32 speed = 10.0f * dt;
        v3 camera_right = cross(make_v3(0, 1, 0), camera_dir);
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
        m4x4 view = look_at(camera_pos, add3(camera_pos, camera_dir), make_v3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, (f32 *)view.e);

        glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glUniform1f(glGetUniformLocation(program, "time"), time);

        glfwSwapBuffers(window);
        glfwPollEvents();
        prev_mouse_pos = mouse_pos;
        prev_time = time;
    }

    glfwTerminate();
	free(arena);
    return 0;
}
