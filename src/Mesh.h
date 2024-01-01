#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include <memory>

#include "MyUtils.h"
#include "Pipelines.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct MeshInstanceUniformBlock
{
	glm::vec4 color;
	glm::mat4 model_matrix;
	glm::vec4 material_factors;
};

class Mesh : public ITrash
{
private:
	VkBuffer vertices = VK_NULL_HANDLE;
	VkBuffer indices = VK_NULL_HANDLE;
	uint32_t index_count;

public:
	Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

	void bind(VkCommandBuffer cmd_buffer);
	void draw(VkCommandBuffer cmd_buffer);

	void destroy();
};

class MeshInstance
{
private:
	MeshInstanceUniformBlock uniform_block = {
		.color = {1.0, 1.0, 1.0, 1.0},
		.model_matrix = glm::mat4(1.0),
		.material_factors = {0.05, 1.0, 1.0, 10.0},
	};
	VkBuffer uniform_buffer = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	UniformBufferSlot uniform_slot = {};
	PipelineMatrixManager::Shader shader = PipelineMatrixManager::Shader::Phong;

public:
	std::shared_ptr<Mesh> mesh = nullptr;

	MeshInstance(std::shared_ptr<Mesh> mesh, PipelineMatrixManager::Shader shader);

	void init_uniforms(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_layout, uint32_t binding, VkBuffer uniform_buffer, UniformBufferSlot slot);
	void set_uniforms(MeshInstanceUniformBlock data);
	void bind_uniforms(VkCommandBuffer cmd_buffer, VkPipelineLayout pipeline_layout);
	VkDescriptorSet get_descriptor_set();
	PipelineMatrixManager::Shader get_shader()
	{
		return shader;
	}
};

class BezierCurve
{
private:
	std::vector<float> coefficients;
	std::vector<float> derivative_coefficients;
	std::vector<glm::vec3> points;
	std::vector<glm::vec3> derivative_points;

	void generate_coefficients();
	uint64_t binom(uint32_t n, uint32_t k);
	uint64_t fact(uint32_t n);
	float power_at(float t, int n, int i);

public:
	BezierCurve(std::vector<glm::vec3> points);

	glm::vec3 value_at(float t);
	glm::vec3 tanget_at(float t);
};

std::unique_ptr<Mesh> create_cube_mesh(float width, float height, float depth, glm::vec3 color);
std::unique_ptr<Mesh> create_cornell_mesh(float width, float height, float depth);
std::unique_ptr<Mesh> create_cylinder_mesh(float radius, float height, int segments, glm::vec3 color);
std::unique_ptr<Mesh> create_sphere_mesh(float radius, int rings, int segments, glm::vec3 color);
std::unique_ptr<Mesh> create_bezier_mesh(std::unique_ptr<BezierCurve> curve, glm::vec3 up, float radius, int resolution, int segments, glm::vec3 color);