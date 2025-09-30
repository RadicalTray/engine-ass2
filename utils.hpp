#pragma once
#include <fstream>
#include <sstream>
#include <string>

using glm::mat4, glm::vec2, glm::vec3, glm::vec4, glm::uvec2;
namespace chrono = std::chrono;

typedef unsigned char uchar;
typedef uint32_t uint;
typedef size_t usize;

struct Position {
	float x;
	float y;
	float z;
};

struct Color {
	float r;
	float g;
	float b;
	float a;
};

struct TexCoord {
	float u;
	float v;
};

struct Vertex {
	vec3 pos;
	vec4 clr;
	vec3 norm;
	vec2 uv;
};

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
std::string readFile(const char *const filepath);

GLFWwindow* init() {
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(1600, 900, "uwu", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(-1);
	}
	std::cout << "GL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

	// first window doesn't trigger framebufferSizeCallback
	// or has the wrong viewport for some reason
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebufferSizeCallback(window, width, height);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugMessageCallback, 0);

	return window;
}

void deinit(GLFWwindow** window) {
	glfwTerminate();
	*window = nullptr;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void GLAPIENTRY
debugMessageCallback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) {
	// WARN: assuming message is always NUL terminated. (can be checked with `length`)
	std::string source_str, type_str, severity_str;
	switch (source) {
		case GL_DEBUG_SOURCE_API:		source_str = "api"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:	source_str = "window system"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	source_str = "shader compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:	source_str = "third party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:	source_str = "application"; break;
		case GL_DEBUG_SOURCE_OTHER:		source_str = "other"; break;
	}
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:			type_str = "error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:		type_str = "deprecated behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:		type_str = "undefined behaviour"; break; 
		case GL_DEBUG_TYPE_PORTABILITY:			type_str = "portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:			type_str = "performance"; break;
		case GL_DEBUG_TYPE_MARKER:			type_str = "marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:			type_str = "push group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:			type_str = "pop group"; break;
		case GL_DEBUG_TYPE_OTHER:			type_str = "other"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:		severity_str = "high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:		severity_str = "medium"; break;
		case GL_DEBUG_SEVERITY_LOW:		severity_str = "low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "notification"; break;
	}
	std::cerr << "GL(" << type_str << ", " << source_str << ", " << severity_str << ", " << id << "): " << message << std::endl;
}

void allocBuffers(usize va_len, uint* va, usize b_len, uint* b) {
	if (va != nullptr) glCreateVertexArrays(va_len, va);
	if (b != nullptr) glCreateBuffers(b_len, b);
}

void freeBuffers(usize va_len, uint* va, usize b_len, uint* b) {
	if (va != nullptr) glDeleteVertexArrays(va_len, va);
	if (b != nullptr) glDeleteBuffers(b_len, b);
}

void loadImagesToTexture2Ds(const usize len, const char *const *const filenames, const uint *const targets) {
	stbi_set_flip_vertically_on_load(true); // opengl/glfw dum dum
	for (usize i = 0; i < len; i++) {
		const char *const filename = filenames[i];
		const uint target = targets[i];

		int width, height, nrChannels;
		uchar *data = stbi_load(filename, &width, &height, &nrChannels, 4);
		if (data) {
			glTextureStorage2D(target, 1, GL_RGBA8, width, height);
			glTextureSubImage2D(target, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
		} else {
			std::cout << "Failed to load texture!" << std::endl;
		}
		stbi_image_free(data);
	}
}

uint createShader(const char *const vert_filename, const char *const frag_filename) {
	const std::string vert_src = readFile(vert_filename), frag_src = readFile(frag_filename);
	const char *vert_src_c = vert_src.data(), *frag_src_c = frag_src.data();

	uint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vert_src_c, NULL);
	glCompileShader(vertex_shader);

	uint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &frag_src_c, NULL);
	glCompileShader(fragment_shader);

	uint shader = glCreateProgram();
	glAttachShader(shader, vertex_shader);
	glAttachShader(shader, fragment_shader);
	glLinkProgram(shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader;
}

std::string readFile(const char *const filepath) {
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	file.open(filepath);
	std::stringstream stream;
	stream << file.rdbuf();
	file.close();
	return stream.str();
}
