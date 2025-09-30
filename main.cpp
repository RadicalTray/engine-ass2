#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <array>

#include "utils.hpp"

using glm::mat4, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::ivec2;
namespace chrono = std::chrono;

struct Mouse {
	double last_xpos;
	double last_ypos;
	double sens;
	float yaw;
	float pitch;
};

struct UniformBuffer {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 model_it;
	vec4 view_pos;
	vec4 light_pos;
	vec4 light_clr;
	vec4 ambient_clr;
	float ambient_str;
};

struct View {
	vec3 pos;
	vec3 front;
	vec3 up;
	float fov;
	float speed;
};

struct Keys {
	bool left_click;
	bool right_click;
	bool w;
	bool s;
	bool a;
	bool d;
	bool e;
	bool q;
	bool space;
	bool shift;
	bool tab;
	bool red;
	bool green;
	bool blue;
};

enum Mode {
	CAM,
	LIGHT,
};

struct State {
	Mouse mouse;
	UniformBuffer ub;
	View view;
	vec3 cam_pos;
	ivec2 scr_res;
	int faces;
	float dt;
	float rot_speed;
	float rot;
	Keys keys;
	Mode mode;

	static State init(GLFWwindow *const window) {
		State state = {};
		state.faces = 4;
		state.rot_speed = 0.04f;
		state.mouse = {
			.sens = 0.1f,
			.yaw = -90.0f,
		};
		state.mode = CAM;
		state.cam_pos = vec3(0.0f, 0.0f, 2.0f),
		state.view = {
			.pos = state.cam_pos,
			.front = vec3(0.0f, 0.0f, -1.0f),
			.up = vec3(0.0f, 1.0f, 0.0f),
			.fov = 90.0f,
			.speed = 0.004f,
		};
		state.ub = {
			.light_pos = vec4(1.2f, 1.0f, 2.0f, 0.2f),
			.light_clr = vec4(1.0f),
			.ambient_clr = vec4(1.0f),
			.ambient_str = 0.1f,
		};
		glfwGetWindowSize(window, &state.scr_res.x, &state.scr_res.y);
		glfwGetCursorPos(window, &state.mouse.last_xpos, &state.mouse.last_ypos);
		state.updateUB();
		return state;
	}

	void updateUB() {
		this->ub.projection = glm::perspective(glm::radians(this->view.fov), (float)this->scr_res.x / (float)this->scr_res.y, 0.1f, 100.0f);
		this->ub.view = glm::lookAt(this->view.pos, this->view.pos + this->view.front, this->view.up);
		this->ub.model = glm::rotate(mat4(1.0f), glm::radians(this->rot), vec3(0.0f, 1.0f, 0.0f));
		this->ub.model_it = glm::transpose(glm::inverse(this->ub.model));
		this->ub.view_pos = vec4(this->view.pos, 0.0f);
	}

	void uploadUB(uint ubo) const {
		glNamedBufferSubData(ubo, 0, sizeof(UniformBuffer), &this->ub);
	}
};

std::vector<Vertex> genVerts(uint vbo, int faces);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void windowSizeCallback(GLFWwindow* window, int width, int height);

int main() {
	GLFWwindow *window = init();
	State state = State::init(window);
	glfwSetWindowUserPointer(window, reinterpret_cast<void *>(&state));
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize buffers
	std::array<uint, 1> va{};
	std::array<uint, 3> b{};
	allocBuffers(va.size(), va.data(), b.size(), b.data());
	const uint vao = va[0];
	const uint vbo = b[0];
	const uint ebo = b[1];
	const uint ubo = b[2];

	glVertexArrayElementBuffer(vao, ebo);
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
	glVertexArrayAttribBinding(vao, 0, 0);
	glEnableVertexArrayAttrib(vao, 1);
	glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, clr));
	glVertexArrayAttribBinding(vao, 1, 0);
	glEnableVertexArrayAttrib(vao, 2);
	glVertexArrayAttribFormat(vao, 2, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, norm));
	glVertexArrayAttribBinding(vao, 2, 0);
	glEnableVertexArrayAttrib(vao, 3);
	glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
	glVertexArrayAttribBinding(vao, 3, 0);

	std::vector<Vertex> vertices = genVerts(vbo, state.faces);
	const std::array<uint, 0> indices = {}; // not in use
	glNamedBufferData(ebo, sizeof(uint)*indices.size(), indices.data(), GL_STATIC_DRAW);
	glNamedBufferData(ubo, sizeof(UniformBuffer), &state.ub, GL_DYNAMIC_DRAW);

	// Initialize shaders
	const uint shader = createShader("./3d.vert", "./3d.frag");
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	State prev_state = state;
	auto start = chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		prev_state = state;
		auto now = chrono::steady_clock::now();
		state.dt = chrono::duration_cast<chrono::milliseconds>(now - start).count();
		start = now;

		{ // process
			glfwPollEvents();
			const float dt = state.dt;
			const float clr_speed = 0.0005f;

			if (state.keys.tab) {
				switch (state.mode) {
				case LIGHT:
					state.mode = CAM;
					state.view.pos = state.cam_pos;
					break;
				case CAM:
					state.mode = LIGHT;
					state.view.pos = state.ub.light_pos;
					break;
				}
				state.keys.tab = false;
			}

			if (state.keys.w) state.view.pos += state.view.front * state.view.speed * dt;
			if (state.keys.s) state.view.pos -= state.view.front * state.view.speed * dt;
			if (state.keys.a) state.view.pos -= glm::normalize(glm::cross(state.view.front, state.view.up)) * state.view.speed * dt;
			if (state.keys.d) state.view.pos += glm::normalize(glm::cross(state.view.front, state.view.up)) * state.view.speed * dt;
			if (state.keys.space) state.view.pos += state.view.up * state.view.speed * dt;
			if (state.keys.shift) state.view.pos -= state.view.up * state.view.speed * dt;
			switch (state.mode) {
			case LIGHT:
				state.ub.light_pos = vec4(state.view.pos, state.ub.light_pos.w);
				if (state.keys.left_click) {
					state.ub.ambient_str += clr_speed * dt;
				}
				if (state.keys.right_click) {
					state.ub.ambient_str -= clr_speed * dt;
				}
				break;
			case CAM:
				state.cam_pos = vec3(state.view.pos);
				if (state.keys.left_click) {
					state.faces += 1;
					vertices = genVerts(vbo, state.faces);
					state.keys.left_click = false;
				}
				if (state.keys.right_click) {
					if (state.faces > 3) state.faces -= 1;
					vertices = genVerts(vbo, state.faces);
					state.keys.right_click = false;
				}
				break;
			}

			if (state.keys.q) state.rot -= state.rot_speed * dt;
			if (state.keys.e) state.rot += state.rot_speed * dt;

			if (state.keys.red) {
				state.ub.light_clr.x += clr_speed * dt;
				while (state.ub.light_clr.x > 1.0f) state.ub.light_clr.x -= 1.0f;
			}
			if (state.keys.green) {
				state.ub.light_clr.y += clr_speed * dt;
				while (state.ub.light_clr.y > 1.0f) state.ub.light_clr.y -= 1.0f;
			}
			if (state.keys.blue) {
				state.ub.light_clr.z += clr_speed * dt;
				while (state.ub.light_clr.z > 1.0f) state.ub.light_clr.z -= 1.0f;
			}
			state.updateUB();
			state.uploadUB(ubo);
		}
		{ // render
			glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(shader);
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}
		glfwSwapBuffers(window);
	}

	glDeleteProgram(shader);
	freeBuffers(va.size(), va.data(), b.size(), b.data());
	deinit(&window);
	return 0;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	switch (key) {
	case GLFW_KEY_W:
		switch (action) {
		case GLFW_PRESS:
			state->keys.w = true;
			break;
		case GLFW_RELEASE:
			state->keys.w = false;
			break;
		}
		break;
	case GLFW_KEY_S:
		switch (action) {
		case GLFW_PRESS:
			state->keys.s = true;
			break;
		case GLFW_RELEASE:
			state->keys.s = false;
			break;
		}
		break;
	case GLFW_KEY_A:
		switch (action) {
		case GLFW_PRESS:
			state->keys.a = true;
			break;
		case GLFW_RELEASE:
			state->keys.a = false;
			break;
		}
		break;
	case GLFW_KEY_D:
		switch (action) {
		case GLFW_PRESS:
			state->keys.d = true;
			break;
		case GLFW_RELEASE:
			state->keys.d = false;
			break;
		}
		break;
	case GLFW_KEY_E:
		switch (action) {
		case GLFW_PRESS:
			state->keys.e = true;
			break;
		case GLFW_RELEASE:
			state->keys.e = false;
			break;
		}
		break;
	case GLFW_KEY_Q:
		switch (action) {
		case GLFW_PRESS:
			state->keys.q = true;
			break;
		case GLFW_RELEASE:
			state->keys.q = false;
			break;
		}
		break;
	case GLFW_KEY_SPACE:
		switch (action) {
		case GLFW_PRESS:
			state->keys.space = true;
			break;
		case GLFW_RELEASE:
			state->keys.space = false;
			break;
		}
		break;
	case GLFW_KEY_LEFT_SHIFT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.shift = true;
			break;
		case GLFW_RELEASE:
			state->keys.shift = false;
			break;
		}
		break;
	case GLFW_KEY_1:
		switch (action) {
		case GLFW_PRESS:
			state->keys.red = true;
			break;
		case GLFW_RELEASE:
			state->keys.red = false;
			break;
		}
		break;
	case GLFW_KEY_2:
		switch (action) {
		case GLFW_PRESS:
			state->keys.green = true;
			break;
		case GLFW_RELEASE:
			state->keys.green = false;
			break;
		}
		break;
	case GLFW_KEY_3:
		switch (action) {
		case GLFW_PRESS:
			state->keys.blue = true;
			break;
		case GLFW_RELEASE:
			state->keys.blue = false;
			break;
		}
		break;
	case GLFW_KEY_TAB:
		switch (action) {
		case GLFW_PRESS:
			state->keys.tab = true;
			break;
		case GLFW_RELEASE:
			state->keys.tab = false;
			break;
		}
		break;
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.left_click = true;
			break;
		case GLFW_RELEASE:
			state->keys.left_click = false;
			break;
		}
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.right_click = true;
			break;
		case GLFW_RELEASE:
			state->keys.right_click = false;
			break;
		}
		break;
	}
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	double xoffset =  (xpos - state->mouse.last_xpos) * state->mouse.sens;
	double yoffset = -(ypos - state->mouse.last_ypos) * state->mouse.sens;
	state->mouse.last_xpos = xpos;
	state->mouse.last_ypos = ypos;

	state->mouse.yaw = state->mouse.yaw + xoffset;
	state->mouse.pitch = std::clamp(state->mouse.pitch + yoffset, -89.0d, 89.0d);
	state->view.front = glm::normalize(vec3(
		std::cos(glm::radians(state->mouse.yaw)) * std::cos(glm::radians(state->mouse.pitch)),
		std::sin(glm::radians(state->mouse.pitch)),
		std::sin(glm::radians(state->mouse.yaw)) * std::cos(glm::radians(state->mouse.pitch))
	));
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	state->scr_res = ivec2(width, height);
}

std::vector<Vertex> genVerts(uint vbo, int faces) {
	std::vector<Vertex> vertices;
	vertices.reserve(faces*6 + (faces-2)*3*2);

	const float pi = M_PI;
	const float radius = 1.0f;
	const float height = 1.0f;
	const float bot = 0.0f - height/2.0f;
	const float top = 0.0f + height/2.0f;
	for (int i = 0; i < faces; i++) { // side
		// in (x, z)
		const vec2 left  = radius * vec2(std::sin((float)    i/(float)faces * 2.0f*pi), std::cos((float)    i/(float)faces * 2.0f*pi));
		const vec2 right = radius * vec2(std::sin((float)(i+1)/(float)faces * 2.0f*pi), std::cos((float)(i+1)/(float)faces * 2.0f*pi));
		const vec3 norm = glm::normalize(glm::cross(
			vec3(left.x,  top, left.y) - vec3(right.x, top, right.y),
                        vec3(left.x,  bot, left.y) - vec3(left.x,  top, left.y)
		));
		vertices.push_back(Vertex{ .pos = vec3(right.x, top, right.y), .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(left.x,  top, left.y),  .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(left.x,  bot, left.y),  .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(left.x,  bot, left.y),  .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(right.x, bot, right.y), .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(right.x, top, right.y), .norm = norm });
	}
	// in (x, z)
	const vec2 origin = radius * vec2(std::sin(0.0f/(float)faces * 2.0f*pi), std::cos(0.0f/(float)faces * 2.0f*pi));
	for (int i = 1; i < faces - 1; i++) { // top
		// in (x, z)
		const vec2 left  = radius * vec2(std::sin((float)    i/(float)faces * 2.0f*pi), std::cos((float)    i/(float)faces * 2.0f*pi));
		const vec2 right = radius * vec2(std::sin((float)(i+1)/(float)faces * 2.0f*pi), std::cos((float)(i+1)/(float)faces * 2.0f*pi));
		const vec3 norm = vec3(0.0f, 1.0f, 0.0f);
		vertices.push_back(Vertex{ .pos = vec3(origin.x, top, origin.y), .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(left.x,   top, left.y),  .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(right.x,  top, right.y),  .norm = norm });
	}
	for (int i = 0; i < faces - 1; i++) { // bot
		// in (x, z)
		const vec2 left  = radius * vec2(std::sin((float)    i/(float)faces * 2.0f*pi), std::cos((float)    i/(float)faces * 2.0f*pi));
		const vec2 right = radius * vec2(std::sin((float)(i+1)/(float)faces * 2.0f*pi), std::cos((float)(i+1)/(float)faces * 2.0f*pi));
		const vec3 norm = vec3(0.0f, -1.0f, 0.0f);
		vertices.push_back(Vertex{ .pos = vec3(origin.x, bot, origin.y), .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(right.x,  bot, right.y),  .norm = norm });
		vertices.push_back(Vertex{ .pos = vec3(left.x,   bot, left.y),  .norm = norm });
	}

	// std::cout << "faces: " << faces << std::endl;
	// std::cout << "verts: " << vertices.size() << std::endl;
	// for (auto v : vertices) {
	// 	std::cout << glm::to_string(v.pos) << " " << glm::to_string(v.norm) << std::endl;
	// }

	glNamedBufferData(vbo, sizeof(Vertex)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
	return vertices;
}
