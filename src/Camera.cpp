#include "Camera.h"

#include <VulkanLaunchpad.h>
#include "INIReader.h"
#include "Input.h"
#include "Utils.h"
#include "Descriptors.h"

#pragma region Camera
Camera::Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles)
{
	this->fovRad = fovRad;
	this->viewportSize = viewportSize;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
	this->position = position;
	this->angles = angles;

	uniform_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(uniform_block), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	updateProjection();
	updateView();
}

void Camera::updateProjection()
{
	float aspect = viewportSize.x / viewportSize.y;
	projectionMatrix = gcgCreatePerspectiveProjectionMatrix(fovRad, aspect, nearPlane, farPlane);
	set_uniforms({projectionMatrix * viewMatrix, glm::vec4(position, 1.0)});
}

void Camera::updateView()
{
	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, position);
	viewMatrix = glm::rotate(viewMatrix, angles.z, {0, 0, 1});
	viewMatrix = glm::rotate(viewMatrix, angles.y, {0, 1, 0});
	viewMatrix = glm::rotate(viewMatrix, angles.x, {1, 0, 0});
	viewMatrix = glm::inverse(viewMatrix);
	set_uniforms({projectionMatrix * viewMatrix, glm::vec4(position, 1.0)});
}

void Camera::init_uniforms(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding)
{
	writeDescriptorSetBuffer(device, descriptor_set, binding, uniform_buffer, sizeof(uniform_block));
	set_uniforms(uniform_block);
}

void Camera::set_uniforms(CameraUniformBlock data)
{
	uniform_block = data;
	if (uniform_buffer != VK_NULL_HANDLE)
		vklCopyDataIntoHostCoherentBuffer(uniform_buffer, &uniform_block, sizeof(uniform_block));
}

void Camera::destroy(VkDevice device)
{
	vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer);
}
#pragma endregion

std::unique_ptr<Camera> createCamera(std::string init_path, GLFWwindow *window)
{
	INIReader camera_ini_reader(init_path);
	double fovDeg = camera_ini_reader.GetReal("camera", "fov", 60);
	double nearPlane = camera_ini_reader.GetReal("camera", "near", 0.1);
	double farPlane = camera_ini_reader.GetReal("camera", "far", 100.0);
	double yaw = camera_ini_reader.GetReal("camera", "yaw", 0.0);
	double pitch = camera_ini_reader.GetReal("camera", "pitch", 0.0);

	int viewportWidth, viewportHeight;
	glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
	return std::make_unique<Camera>(glm::radians(fovDeg), glm::vec2(viewportWidth, viewportHeight), nearPlane, farPlane, glm::vec3(), glm::vec3(pitch, yaw, 0));
}

#pragma region OrbitControls
OrbitControls::OrbitControls(std::shared_ptr<Camera> camera)
{
	azimuth = -camera->angles.y;
	elevation = camera->angles.x;
	this->camera = camera;
}

void OrbitControls::update()
{
	auto input = Input::instance();
	distance -= input->scrollDelta().y / 5.0f;
	distance = glm::clamp(distance, 0.1f, 100.0f);

	if (input->isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
	{
		auto delta = input->mouseDelta();
		azimuth -= delta.x / 200.0f;
		azimuth = glm::mod(glm::mod(azimuth, glm::two_pi<float>()) + glm::two_pi<float>(), glm::two_pi<float>());
		elevation += delta.y / 200.0f;
		elevation = glm::clamp(elevation, -glm::half_pi<float>(), glm::half_pi<float>());
	}

	camera->position = center;
	camera->position += glm::vec3(
		glm::sin(azimuth) * glm::cos(elevation),
		glm::sin(elevation),
		glm::cos(azimuth) * glm::cos(elevation));
	camera->position *= distance;
	camera->angles.x = -1.0f * elevation;
	camera->angles.y = 1.0f * azimuth;
	camera->updateView();
}
#pragma endregion