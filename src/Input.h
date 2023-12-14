#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>

class Input
{
private:
	static std::shared_ptr<Input> _instance;

	float _prevTime;
	float _time;
	float _timeDelta;
	glm::vec2 _mousePrevPos;
	glm::vec2 _mousePos;
	glm::vec2 _mouseDelta;
	glm::vec2 _scrollDelta;
	glm::vec2 _scrollNextDelta;
	bool _mousePrevButtons[GLFW_MOUSE_BUTTON_LAST + 1];
	bool _mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
	bool _keysPrev[GLFW_KEY_LAST + 1];
	bool _keys[GLFW_KEY_LAST + 1];

public:
	const glm::vec2 mousePos() { return _mousePos; }
	const glm::vec2 mouseDelta() { return _mouseDelta; }
	const glm::vec2 scrollDelta() { return _scrollDelta; }
	const glm::vec2 timeDelta() { return _mouseDelta; }

	const bool isMouseDown(int button)
	{
		return _mouseButtons[button];
	}

	const bool isMouseTap(int button)
	{
		return _mouseButtons[button] && !_mousePrevButtons[button];
	}

	const bool isKeyDown(int key)
	{
		return _keys[key];
	}

	const bool isKeyPress(int key)
	{
		return _keys[key] && !_keysPrev[key];
	}

	const bool isKeyRelease(int key)
	{
		return !_keys[key] && _keysPrev[key];
	}

	void update();
	void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
	void onCursorPos(GLFWwindow *window, double x, double y);
	void onMouseButton(GLFWwindow *window, int button, int action, int mods);
	void onScroll(GLFWwindow *window, double dx, double dy);

	static std::shared_ptr<Input> instance();
	static std::shared_ptr<Input> init(GLFWwindow *window);
};