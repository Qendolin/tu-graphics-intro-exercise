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

void Mesh::destroy(VkDevice device)
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
MeshInstance::MeshInstance(std::shared_ptr<Mesh> mesh, PipelineMatrixManager::Shader shader)
{
	this->mesh = mesh;
	this->shader = shader;
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

#pragma region MeshBuilder
class MeshBuilder
{
private:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<glm::mat4> transforms = {glm::mat4(1.0)};
	bool reverse_winding = false;

public:
	class Cycle
	{
	private:
		uint32_t start = -1;
		uint32_t length = -1;

	public:
		Cycle() {}
		Cycle(uint32_t start, uint32_t len) : start(start), length(len) {}

		uint32_t rel(int i)
		{
			if (length <= 0)
				return i;
			return start + ((i % length) + length) % length;
		}
	};

	MeshBuilder()
	{
	}

	std::unique_ptr<Mesh> build()
	{
		return make_unique<Mesh>(vertices, indices);
	}

	uint32_t index()
	{
		return vertices.size() - 1;
	}

	void transform(glm::mat4 m)
	{
		transforms.back() = m * transforms.back();
	}

	void transform(glm::mat3 m)
	{
		transforms.back() = transforms.back() * glm::mat4(m);
	}

	void push_transform()
	{
		transforms.push_back(transforms.back());
	}

	void pop_transform()
	{
		transforms.pop_back();
	}

	void vertex(Vertex v)
	{
		v.position = transforms.back() * glm::vec4(v.position, 1.0);
		v.normal = glm::mat3(transforms.back()) * v.normal;
		vertices.push_back(v);
	}

	// A--B
	// | /
	// C
	void tri(uint32_t a, uint32_t b, uint32_t c)
	{
		if (reverse_winding)
		{
			indices.push_back(a);
			indices.push_back(c);
			indices.push_back(b);
		}
		else
		{
			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(c);
		}
	}

	//	A--B
	//	| /|
	//	C--D
	void quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
	{
		tri(a, b, c);
		tri(d, c, b);
	}

	Cycle start_cycle(uint32_t length)
	{
		return Cycle(vertices.size(), length);
	}

	void winding(bool reverse)
	{
		reverse_winding = reverse;
	}
};
#pragma endregion

void append_circle_cap(MeshBuilder &builder, float radius, int segments, glm::vec3 pos, glm::vec3 color, glm::vec3 normal)
{
	builder.vertex({pos, color, normal, {0.5, 0.5}});
	int center_index = builder.index();
	auto cycle = builder.start_cycle(segments);
	for (int s = 0; s < segments; s++)
	{
		float phi = glm::two_pi<float>() * s / segments;
		glm::vec3 spoke = {
			glm::cos(phi),
			0.0,
			glm::sin(phi),
		};
		glm::vec3 v = pos + radius * spoke;
		glm::vec2 uv = {spoke.x * radius * 2.0f, spoke.z * radius * 2.0f};
		uv = uv * 0.5f + 0.5f;
		uv.y = 1.0 - uv.y;

		builder.vertex({v, color, normal, uv});
		builder.tri(center_index, cycle.rel(s), cycle.rel(s + 1));
	}
}

std::unique_ptr<Mesh> create_cylinder_mesh(float radius, float height, int segments, glm::vec3 color)
{
	std::unique_ptr<MeshBuilder> builder = std::make_unique<MeshBuilder>();

	append_circle_cap(*builder, radius, segments, {0, -height / 2, 0}, color, {0, -1, 0});
	builder->winding(true);
	append_circle_cap(*builder, radius, segments, {0, height / 2, 0}, color, {0, 1, 0});
	builder->winding(false);

	MeshBuilder::Cycle bot_cycle = builder->start_cycle(segments + 1);
	MeshBuilder::Cycle top_cycle;

	for (int half = 0; half < 2; half++)
	{
		bool top = half == 1;
		for (int s = 0; s <= segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;
			glm::vec3 n = {
				glm::cos(phi),
				0.0,
				glm::sin(phi),
			};
			glm::vec3 v = radius * n;
			v.y = top ? height / 2 : -height / 2;
			glm::vec2 uv = {1.0 - ((float)s / segments), 1.0 - half};

			builder->vertex({v, color, n, uv});

			if (top)
				builder->quad(top_cycle.rel(s), top_cycle.rel(s + 1), bot_cycle.rel(s), bot_cycle.rel(s + 1));
		}
		if (!top)
			top_cycle = builder->start_cycle(segments + 1);
	}

	return builder->build();
}

std::unique_ptr<Mesh> create_sphere_mesh(float radius, int rings, int segments, glm::vec3 color)
{
	std::unique_ptr<MeshBuilder> builder = std::make_unique<MeshBuilder>();

	uint32_t cap_index = 0;
	for (int s = 0; s <= segments; s++)
	{
		builder->vertex({{0, -radius, 0}, color, {0, -1, 0}, {1.0 - ((float)s / segments), 1.0}});
		builder->vertex({{0, radius, 0}, color, {0, 1, 0}, {1.0 - ((float)s / segments), 0.0}});
	}

	MeshBuilder::Cycle prev_cycle;
	for (int r = 1; r < rings; r++)
	{
		bool cap = r == 1 || r == rings - 1;
		bool top_cap = r == rings - 1;
		float theta = glm::pi<float>() * r / rings;
		auto curr_cycle = builder->start_cycle(segments + 1);
		for (int s = 0; s <= segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;

			glm::vec3 n = {
				glm::sin(theta) * glm::cos(phi),
				-glm::cos(theta),
				glm::sin(theta) * glm::sin(phi),
			};
			glm::vec3 v = radius * n;
			glm::vec2 uv = {1.0 - ((float)s / segments), 1.0 - ((float)r / rings)};

			builder->vertex({v, color, n, uv});

			if (cap)
			{
				if (top_cap)
					builder->tri(cap_index + s * 2 + 1, curr_cycle.rel(s + 1), curr_cycle.rel(s));
				else
					builder->tri(cap_index + s * 2, curr_cycle.rel(s), curr_cycle.rel(s + 1));
			}
			if (r > 1)
			{
				builder->quad(curr_cycle.rel(s), curr_cycle.rel(s + 1), prev_cycle.rel(s), prev_cycle.rel(s + 1));
			}
		}
		prev_cycle = curr_cycle;
	}

	return builder->build();
}

std::unique_ptr<Mesh> create_bezier_mesh(std::unique_ptr<BezierCurve> curve, glm::vec3 up, float radius, int resolution, int segments, glm::vec3 color)
{
	std::unique_ptr<MeshBuilder> builder = std::make_unique<MeshBuilder>();

	for (int cap = 0; cap <= 1; cap++)
	{
		float f = float(cap);
		builder->push_transform();
		glm::vec3 tan = glm::normalize(curve->tanget_at(f));
		glm::vec3 bitan = glm::normalize(glm::cross(tan, up));
		glm::vec3 norm = glm::cross(bitan, tan);
		glm::mat4 cap_mat = glm::translate(glm::mat4(1.0), curve->value_at(f)) * glm::mat4(glm::mat3(bitan, tan, norm));
		builder->transform(cap_mat);
		builder->winding(cap == 1);
		append_circle_cap(*builder, radius, segments, {0, 0, 0}, color, {0, cap == 0 ? -1 : 1, 0});
		builder->pop_transform();
	}
	builder->winding(false);

	MeshBuilder::Cycle prev_cycle;
	float len = 0.0;
	glm::vec3 prev_p;
	for (int r = 0; r <= resolution; r++)
	{
		bool cap = r == 0 || r == resolution;
		bool top_cap = r == resolution;
		float f = float(r) / resolution;
		glm::vec3 p = curve->value_at(f);
		glm::vec3 tan = glm::normalize(curve->tanget_at(f));
		glm::vec3 bitan = glm::normalize(glm::cross(tan, up));
		auto curr_cycle = builder->start_cycle(segments + 1);

		if (r > 0)
		{
			len += glm::distance(prev_p, p);
		}

		for (int s = 0; s <= segments; s++)
		{
			float phi = glm::two_pi<float>() * s / segments;
			glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), phi, tan)) * bitan;
			glm::vec3 v = p + n * radius;
			glm::vec2 uv = {(float)s / segments, len};

			builder->vertex({v, color, n, uv});

			if (r > 0)
			{
				builder->quad(prev_cycle.rel(s), prev_cycle.rel(s + 1), curr_cycle.rel(s), curr_cycle.rel(s + 1));
			}
		}
		prev_cycle = curr_cycle;
		prev_p = p;
	}

	return builder->build();
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

glm::vec3 cube_face_normals[]{
	{0.0, 1.0, 0.0},  // top
	{0.0, -1.0, 0.0}, // bottom
	{-1.0, 0.0, 0.0}, // left
	{1.0, 0.0, 0.0},  // right
	{0.0, 0.0, 1.0},  // front
	{0.0, 0.0, -1.0}  // back
};

glm::vec2 cube_uvs[]{
	{0.0, 0.0},
	{1.0, 0.0},
	{1.0, 1.0},
	{0.0, 1.0},
};

struct CubeFace
{
	uint32_t face;
	uint32_t verts[4];
};

std::vector<CubeFace>
	cube_faces = {
		{
			0, // Top
			{6, 7, 3, 2},
		},
		{
			1, // Bottom
			{0, 1, 5, 4},
		},
		{
			2, // Left
			{6, 2, 0, 4},
		},
		{
			3, // Right
			{3, 7, 5, 1},
		},
		{
			4, // Front
			{2, 3, 1, 0},
		},
		{
			5, // Back
			{7, 6, 4, 5},
		},
};

std::unique_ptr<Mesh> create_cube_mesh(float width, float height, float depth, glm::vec3 color)
{
	std::vector<glm::vec3> positions(std::begin(cube_vertex_positions), std::end(cube_vertex_positions));

	auto scale = glm::mat4(1.0);
	scale = glm::scale(scale, {width, height, depth});

	for (size_t i = 0; i < positions.size(); i++)
	{
		positions[i] = glm::vec3(scale * glm::vec4(positions[i], 1.0f));
	}

	auto vertices = std::vector<Vertex>(cube_faces.size() * 4);
	auto indices = std::vector<uint32_t>();
	indices.reserve(cube_faces.size() * 6);
	uint32_t index = 0;
	for (size_t i = 0; i < cube_faces.size(); i++)
	{
		auto verts = cube_faces[i].verts;
		vertices[i * 4 + 0] = {positions[verts[0]], color, cube_face_normals[i], cube_uvs[0]};
		vertices[i * 4 + 1] = {positions[verts[1]], color, cube_face_normals[i], cube_uvs[1]};
		vertices[i * 4 + 2] = {positions[verts[2]], color, cube_face_normals[i], cube_uvs[2]};
		vertices[i * 4 + 3] = {positions[verts[3]], color, cube_face_normals[i], cube_uvs[3]};
		indices.insert(indices.end(), {index + 0, index + 2, index + 1, index + 2, index + 0, index + 3});
		index += 4;
	}

	return std::make_unique<Mesh>(vertices, indices);
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

glm::vec3 cornell_vertex_normals[]{
	{0.0, -1.0, 0.0}, // top
	{0.0, 1.0, 0.0},  // bottom
	{1.0, 0.0, 0.0},  // left
	{-1.0, 0.0, 0.0}, // right
	{0.0, 0.0, 1.0}	  // back
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
			vertices[i].normal = cornell_vertex_normals[face];
		}
	}

	return std::make_unique<Mesh>(vertices, cornell_indices);
}