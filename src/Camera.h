#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "MyUtils.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <string>

struct CameraUniformBlock
{
	glm::mat4 view_projection_matrix;
};

class Camera : public ITrash
{
private:
	CameraUniformBlock uniform_block;
	VkBuffer uniform_buffer = VK_NULL_HANDLE;

public:
	float fovRad;
	glm::vec2 viewportSize;
	float nearPlane;
	float farPlane;
	glm::vec3 position;
	// pitch, yaw, roll
	glm::vec3 angles;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles);

	void updateProjection();
	void updateView();
	void init_uniforms(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding);
	void set_uniforms(CameraUniformBlock data);

	void destroy();
};

std::unique_ptr<Camera> createCamera(std::string init_path, GLFWwindow *window);

class OrbitControls
{
private:
	float azimuth = 0.0f;
	float elevation = 0.0f;
	float distance = 5.0f;
	glm::vec3 center = glm::vec3(0.0f);
	std::shared_ptr<Camera> camera = nullptr;

public:
	OrbitControls(std::shared_ptr<Camera> camera);

	void update();
};