#include "Mesh.h"

#include <VulkanLaunchpad.h>
#include "Descriptors.h"

#pragma region Mesh
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
{
	this->vertices = vklCreateHostCoherentBufferWithBackingMemory(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vklCopyDataIntoHostCoherentBuffer(this->vertices, &vertices.front(), vertices.size() * sizeof(Vertex));
	this->indices = vklCreateHostCoherentBufferWithBackingMemory(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	vklCopyDataIntoHostCoherentBuffer(this->indices, &indices.front(), indices.size() * sizeof(uint32_t));
	this->index_count = indices.size();
}

void Mesh::destroy()
{
	vklDestroyHostCoherentBufferAndItsBackingMemory(vertices);
	vklDestroyHostCoherentBufferAndItsBackingMemory(indices);
}

void Mesh::bind(VkCommandBuffer cmd_buffer)
{
	VkDeviceSize vertex_offset = 0;
	vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertices, &vertex_offset);
	vkCmdBindIndexBuffer(cmd_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer cmd_buffer)
{
	vkCmdDrawIndexed(cmd_buffer, index_count, 1, 0, 0, 0);
}
#pragma endregion

#pragma region MeshInstance
MeshInstance::MeshInstance(std::shared_ptr<Mesh> mesh)
{
	this->mesh = mesh;
}

void MeshInstance::init_uniforms(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_layout, uint32_t binding, VkBuffer uniform_buffer, UniformBufferSlot slot)
{
	this->descriptor_set = createVkDescriptorSet(device, descriptor_pool, descriptor_layout);
	writeDescriptorSetBuffer(device, descriptor_set, binding, uniform_buffer, sizeof(uniform_block), slot);
	this->uniform_buffer = uniform_buffer;
	this->uniform_slot = slot;
	set_uniforms(uniform_block);
}

void MeshInstance::set_uniforms(MeshInstanceUniformBlock data)
{
	uniform_block = data;
	if (uniform_buffer != VK_NULL_HANDLE)
		vklCopyDataIntoHostCoherentBuffer(uniform_buffer, uniform_slot.offset, &uniform_block, uniform_slot.size);
}

void MeshInstance::bind_uniforms(VkCommandBuffer cmd_buffer, VkPipelineLayout pipeline_layout)
{
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
}

VkDescriptorSet MeshInstance::get_descriptor_set()
{
	return descriptor_set;
}
#pragma endregion

#pragma region BezierCurve
void BezierCurve::generate_coefficients()
{
	uint32_t n = points.size() - 1;
	for (uint32_t i = 0; i <= n; i++)
	{
		coefficients.push_back(binom(n, i));
		if (i <= n - 1)
		{
			derivative_coefficients.push_back(binom(n - 1, i));
			derivative_points.push_back(points[i + 1] - points[i]);
		}
	}
}

uint64_t BezierCurve::binom(uint32_t n, uint32_t k)
{
	return fact(n) / (fact(k) * fact(n - k));
}

uint64_t BezierCurve::fact(uint32_t n)
{
	uint64_t factorial = 1;
	for (int i = 1; i <= n; ++i)
	{
		factorial *= i;
	}
	return factorial;
}

float BezierCurve::power_at(float t, int n, int i)
{
	if (i == 0)
		return std::pow(1.0 - t, n - i);
	if (i == n)
		return std::pow(t, i);
	return std::pow(1.0 - t, n - i) * std::pow(t, i);
}

BezierCurve::BezierCurve(std::vector<glm::vec3> points)
{
	this->points = points;
	this->generate_coefficients();
}

glm::vec3 BezierCurve::value_at(float t)
{
	glm::vec3 sum = {};
	uint32_t n = points.size() - 1;
	for (size_t i = 0; i <= n; i++)
	{
		float w = coefficients[i] * power_at(t, n, i);
		sum += w * points[i];
	}
	return sum;
}

glm::vec3 BezierCurve::tanget_at(float t)
{
	glm::vec3 sum = {};
	uint32_t n = points.size() - 1;
	for (size_t i = 0; i <= n - 1; i++)
	{
		float w = derivative_coefficients[i] * power_at(t, n - 1, i);
		sum += w * derivative_points[i];
	}
	return float(n) * sum;
}
#pragma endregion

std::unique_ptr<Mesh> create_cylinder_mesh(float radius, float height, int segments, glm::vec3 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.push_back({{0, -height / 2, 0}, color});
	vertices.push_back({{0, height / 2, 0}, color});

	const int bot_start = 2;
	const int bot_end = 2 + segments - 1;
	const int top_start = 2 + segments;
	const int top_end = 2 + segments * 2 - 1;

	for (int half = 0; half < 2; half++)
	{
		bool top = half == 1;
		for (int s = 0; s < segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;
			glm::vec3 v = {
				glm::cos(phi) * radius,
				top ? height / 2 : -height / 2,
				glm::sin(phi) * radius,
			};

			vertices.push_back({v, color});

			if (top && s > 0)
			{
				indices.push_back(0);
				indices.push_back(bot_start + s - 1);
				indices.push_back(bot_start + s);

				indices.push_back(top_start + s - 1);
				indices.push_back(bot_start + s);
				indices.push_back(bot_start + s - 1);

				indices.push_back(1);
				indices.push_back(top_start + s);
				indices.push_back(top_start + s - 1);

				indices.push_back(bot_start + s);
				indices.push_back(top_start + s - 1);
				indices.push_back(top_start + s);
			}
		}
	}

	indices.push_back(0);
	indices.push_back(bot_end);
	indices.push_back(bot_start);

	indices.push_back(top_end);
	indices.push_back(bot_start);
	indices.push_back(bot_end);

	indices.push_back(1);
	indices.push_back(top_start);
	indices.push_back(top_end);

	indices.push_back(bot_start);
	indices.push_back(top_end);
	indices.push_back(top_start);

	return make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> create_sphere_mesh(float radius, int rings, int segments, glm::vec3 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.push_back({{0, -radius, 0}, color});
	vertices.push_back({{0, radius, 0}, color});

	int prev_ring = 0;
	int curr_ring = 2;

	for (int r = 1; r < rings; r++)
	{
		bool cap = r == 1 || r == rings - 1;
		bool top_cap = r == rings - 1;
		float theta = glm::pi<float>() * r / rings;
		for (int s = 0; s < segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;

			glm::vec3 v = {
				radius * glm::sin(theta) * glm::cos(phi),
				radius * -glm::cos(theta),
				radius * glm::sin(theta) * glm::sin(phi),
			};

			vertices.push_back({v, color});

			if (s == 0)
				continue;

			if (cap)
			{
				indices.push_back(curr_ring + s - 1);
				if (top_cap)
					indices.push_back(1);
				indices.push_back(curr_ring + s);
				if (!top_cap)
					indices.push_back(0);
			}
			if (r > 1)
			{
				indices.push_back(curr_ring + s - 1);
				indices.push_back(curr_ring + s);
				indices.push_back(prev_ring + s - 1);

				indices.push_back(prev_ring + s);
				indices.push_back(prev_ring + s - 1);
				indices.push_back(curr_ring + s);
			}
		}
		if (cap)
		{
			indices.push_back(curr_ring + segments - 1);
			if (top_cap)
				indices.push_back(1);
			indices.push_back(curr_ring);
			if (!top_cap)
				indices.push_back(0);
		}
		if (r > 1)
		{
			indices.push_back(curr_ring + segments - 1);
			indices.push_back(curr_ring);
			indices.push_back(prev_ring + segments - 1);

			indices.push_back(prev_ring);
			indices.push_back(prev_ring + segments - 1);
			indices.push_back(curr_ring);
		}
		prev_ring = curr_ring;
		curr_ring += segments;
	}

	return make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> create_bezier_mesh(std::unique_ptr<BezierCurve> curve, glm::vec3 up, float radius, int resolution, int segments, glm::vec3 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.push_back({curve->value_at(1.0), color});
	vertices.push_back({curve->value_at(0.0), color});

	int prev_ring = 0;
	int curr_ring = 2;
	for (int r = 0; r <= resolution; r++)
	{
		bool cap = r == 0 || r == resolution;
		bool top_cap = r == resolution;
		float f = 1.0 - (float(r) / resolution);
		glm::vec3 p = curve->value_at(f);
		glm::vec3 t = curve->tanget_at(f);
		glm::vec3 bn = glm::normalize(glm::cross(t, up)) * radius;
		for (int s = 0; s < segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;
			glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), phi, t)) * bn;
			glm::vec3 v = p + n;
			vertices.push_back({v, color});

			if (s == 0)
				continue;

			if (cap)
			{
				indices.push_back(curr_ring + s - 1);
				if (top_cap)
					indices.push_back(1);
				indices.push_back(curr_ring + s);
				if (!top_cap)
					indices.push_back(0);
			}
			if (r > 0)
			{
				indices.push_back(curr_ring + s - 1);
				indices.push_back(curr_ring + s);
				indices.push_back(prev_ring + s - 1);

				indices.push_back(prev_ring + s);
				indices.push_back(prev_ring + s - 1);
				indices.push_back(curr_ring + s);
			}
		}
		if (cap)
		{
			indices.push_back(curr_ring + segments - 1);
			if (top_cap)
				indices.push_back(1);
			indices.push_back(curr_ring);
			if (!top_cap)
				indices.push_back(0);
		}
		if (r > 0)
		{
			indices.push_back(curr_ring + segments - 1);
			indices.push_back(curr_ring);
			indices.push_back(prev_ring + segments - 1);

			indices.push_back(prev_ring);
			indices.push_back(prev_ring + segments - 1);
			indices.push_back(curr_ring);
		}
		prev_ring = curr_ring;
		curr_ring += segments;
	}

	return make_unique<Mesh>(vertices, indices);
}

glm::vec3 cube_vertex_positions[]{
	{-0.5, -0.5, 0.5},	// 0
	{0.5, -0.5, 0.5},	// 1
	{-0.5, 0.5, 0.5},	// 2
	{0.5, 0.5, 0.5},	// 3
	{-0.5, -0.5, -0.5}, // 4
	{0.5, -0.5, -0.5},	// 5
	{-0.5, 0.5, -0.5},	// 6
	{0.5, 0.5, -0.5},	// 7
};

std::vector<uint32_t> cube_indices = {
	// Top
	7, 6, 2,
	2, 3, 7,
	// Bottom
	0, 4, 5,
	5, 1, 0,
	// Left
	0, 2, 6,
	6, 4, 0,
	// Right
	7, 3, 1,
	1, 5, 7,
	// Front
	3, 2, 0,
	0, 1, 3,
	// Back
	4, 6, 7,
	7, 5, 4};

std::unique_ptr<Mesh> create_cube_mesh(float width, float height, float depth, glm::vec3 color)
{
	std::vector<glm::vec3> positions(std::begin(cube_vertex_positions), std::end(cube_vertex_positions));

	auto scale = glm::mat4(1.0);
	scale = glm::scale(scale, {width, height, depth});

	for (size_t i = 0; i < positions.size(); i++)
	{
		positions[i] = glm::vec3(scale * glm::vec4(positions[i], 1.0f));
	}

	auto vertices = std::vector<Vertex>(positions.size());
	for (size_t i = 0; i < vertices.size(); i++)
	{
		vertices[i] = {positions[i], color};
	}

	return std::make_unique<Mesh>(vertices, cube_indices);
	;
}

std::vector<uint32_t> cornell_indices = {
	// Top
	0, 1, 2,
	2, 3, 0,
	// Bottom
	4, 5, 6,
	6, 7, 4,
	// Left
	8, 9, 10,
	10, 11, 8,
	// Right
	12, 13, 14,
	14, 15, 12,
	// Back
	16, 17, 18,
	18, 19, 16};

glm::vec3 cornell_vertex_colors[]{
	{0.96, 0.93, 0.85}, // Top
	{0.64, 0.64, 0.64}, // Bottom
	{1.0, 0.0, 0.0},	// Left
	{0.0, 1.0, 0.0},	// Right
	{0.76, 0.74, 0.68}	// Back
};

uint32_t cornell_position_swizzle[]{
	// Top
	2, 6, 7, 3,
	// Bottom
	5, 4, 0, 1,
	// Left
	6, 2, 0, 4,
	// Right
	1, 3, 7, 5,
	// Back
	7, 6, 4, 5};

std::unique_ptr<Mesh> create_cornell_mesh(float width, float height, float depth)
{
	std::vector<glm::vec3> positions(std::begin(cube_vertex_positions), std::end(cube_vertex_positions));

	auto scale = glm::mat4(1.0);
	scale = glm::scale(scale, {width, height, depth});

	for (size_t i = 0; i < positions.size(); i++)
	{
		positions[i] = glm::vec3(scale * glm::vec4(positions[i], 1.0f));
	}

	auto vertices = std::vector<Vertex>(5 * 4);
	for (size_t face = 0; face < 5; face++)
	{
		for (size_t v = 0; v < 4; v++)
		{
			size_t i = face * 4 + v;
			vertices[i].position = positions[cornell_position_swizzle[i]];
			vertices[i].color = cornell_vertex_colors[face];
		}
	}

	return std::make_unique<Mesh>(vertices, cornell_indices);
}