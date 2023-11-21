#include "Input.h"

#pragma region Input

void Input::update()
{
	_time = glfwGetTime();
	_timeDelta = _time - _prevTime;
	_mouseDelta = _mousePos - _mousePrevPos;
	_mousePrevPos = _mousePos;
	_scrollDelta = _scrollNextDelta;
	_scrollNextDelta = glm::vec2(0.0f);
	std::copy(std::begin(_mouseButtons), std::end(_mouseButtons), std::begin(_mousePrevButtons));
	std::copy(std::begin(_keys), std::end(_keys), std::begin(_keysPrev));
}

void Input::onKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	_keys[key] = action != GLFW_RELEASE;
}

void Input::onCursorPos(GLFWwindow *window, double x, double y)
{
	_mousePos.x = x;
	_mousePos.y = y;
}

void Input::onMouseButton(GLFWwindow *window, int button, int action, int mods)
{
	_mouseButtons[button] = action != GLFW_RELEASE;
}

void Input::onScroll(GLFWwindow *window, double dx, double dy)
{
	_scrollNextDelta.x += dx;
	_scrollNextDelta.y += dy;
}

std::shared_ptr<Input> Input::instance()
{
	return Input::_instance;
}

std::shared_ptr<Input> Input::init(GLFWwindow *window)
{
	Input::_instance = std::shared_ptr<Input>(new Input());
	glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
					   { Input::_instance->onKey(window, key, scancode, action, mods); });
	glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y)
							 { Input::_instance->onCursorPos(window, x, y); });
	glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods)
							   { Input::_instance->onMouseButton(window, button, action, mods); });
	glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy)
						  { Input::_instance->onScroll(window, dx, dy); });
	return Input::_instance;
}

std::shared_ptr<Input> Input::_instance = nullptr;
#pragma endregion