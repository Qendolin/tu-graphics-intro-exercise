#include "Pipelines.h"

#include "Mesh.h"
#include "Utils.h"
#include "PathUtils.h"
#include "Input.h"

VkPipeline createVkPipeline(PipelineParams &params)
{
	VklGraphicsPipelineConfig graphics_pipeline_config = {
		.vertexShaderPath = params.vertex_shader_path.c_str(),
		.fragmentShaderPath = params.fragment_shader_path.c_str(),
		.vertexInputBuffers = {{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		}},
		.inputAttributeDescriptions = {
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, position),
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, color),
			}},
		.polygonDrawMode = params.polygon_mode,
		.triangleCullingMode = params.culling_mode,
		.descriptorLayout = {{
								 .binding = 0,
								 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								 .descriptorCount = 1,
								 .stageFlags = VK_SHADER_STAGE_ALL,
							 },
							 {
								 .binding = 1,
								 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								 .descriptorCount = 1,
								 .stageFlags = VK_SHADER_STAGE_ALL,
							 }},
	};
	return vklCreateGraphicsPipeline(graphics_pipeline_config);
}

std::vector<std::vector<VkPipeline>> createVkPipelineMatrix(PipelineParams &params, std::vector<VkPolygonMode> &polygonModes, std::vector<VkCullModeFlags> &cullingModes)
{
	auto m = std::vector<std::vector<VkPipeline>>(polygonModes.size());
	for (int i = 0; i < polygonModes.size(); i++)
	{
		m[i] = std::vector<VkPipeline>(cullingModes.size());
		for (int j = 0; j < cullingModes.size(); j++)
		{
			params.polygon_mode = polygonModes[i];
			params.culling_mode = cullingModes[j];
			m[i][j] = createVkPipeline(params);
		}
	}
	return m;
}

void destroyVkPipelineMatrix(std::vector<std::vector<VkPipeline>> matrix)
{
	for (auto &&row : matrix)
	{
		for (auto &&entry : row)
		{
			vklDestroyGraphicsPipeline(entry);
		}
	}
}

#pragma region PipelineMatrixManager
PipelineMatrixManager::PipelineMatrixManager(std::string vshName, std::string fshName)
{
	std::string vertShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/" + vshName);
	std::string fragShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/" + fshName);
	PipelineParams pipelineParams = {
		.vertex_shader_path = vertShaderPath,
		.fragment_shader_path = fragShaderPath,
		.polygon_mode = VK_POLYGON_MODE_FILL,
		.culling_mode = VK_CULL_MODE_NONE,
	};

	matrix = createVkPipelineMatrix(pipelineParams, polygon_modes, culling_modes);
}

void PipelineMatrixManager::destroy()
{
	destroyVkPipelineMatrix(matrix);
}

void PipelineMatrixManager::set_polygon_mode(int mode)
{
	polygon_mode = (mode + polygon_modes.size()) % polygon_modes.size();
}

void PipelineMatrixManager::set_culling_mode(int mode)
{
	culling_mode = (mode + culling_modes.size()) % culling_modes.size();
}

void PipelineMatrixManager::update()
{
	auto input = Input::instance();

	if (input->isKeyTap(GLFW_KEY_F1))
		set_polygon_mode(polygon_mode + 1);

	if (input->isKeyTap(GLFW_KEY_F2))
		set_culling_mode(culling_mode + 1);
}

VkPipeline PipelineMatrixManager::selected()
{
	return matrix[polygon_mode][culling_mode];
}
#pragma endregion

std::unique_ptr<PipelineMatrixManager> createPipelineManager(std::string init_renderer_filepath)
{
	auto manager = std::make_unique<PipelineMatrixManager>("task2.vert", "task2.frag");

	INIReader renderer_reader(init_renderer_filepath);
	bool as_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
	if (as_wireframe)
		manager->set_polygon_mode(1);
	bool with_backface_culling = renderer_reader.GetBoolean("renderer", "backface_culling", false);
	if (with_backface_culling)
		manager->set_culling_mode(1);

	return manager;
}