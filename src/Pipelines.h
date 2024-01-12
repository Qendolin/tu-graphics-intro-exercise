#pragma once

#include <vulkan/vulkan.h>
#include <VulkanLaunchpad.h>

#include "MyUtils.h"
#include "Utils.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>
#include <array>

struct PipelineParams
{
	std::string vertex_shader_path;
	std::string fragment_shader_path;
	VkPolygonMode polygon_mode;
	VkCullModeFlags culling_mode;
};

VkPipeline createVkPipeline(PipelineParams &params);
std::vector<std::vector<VkPipeline>> createVkPipelineMatrix(PipelineParams &params, std::vector<VkPolygonMode> &polygonModes, std::vector<VkCullModeFlags> &cullingModes);
void destroyVkPipelineMatrix(std::vector<std::vector<VkPipeline>> matrix);

class PipelineMatrixManager : public ITrash
{
public:
	enum Shader
	{
		Phong,
		Gouraud,
		Box
	};

private:
	std::vector<VkPolygonMode> polygon_modes = std::vector<VkPolygonMode>({
		VK_POLYGON_MODE_FILL,
		VK_POLYGON_MODE_LINE,
	});
	std::vector<VkCullModeFlags> culling_modes = std::vector<VkCullModeFlags>({
		VK_CULL_MODE_NONE,
		VK_CULL_MODE_BACK_BIT,
		VK_CULL_MODE_FRONT_BIT,
	});
	int polygon_mode = 0;
	int culling_mode = 0;
	Shader shader = Shader::Phong;
	std::array<std::vector<std::vector<VkPipeline>>, 3> matrix;

public:
	PipelineMatrixManager();

	void load(Shader shader, std::string vshName, std::string fshName);
	void destroy(VkDevice device);
	void set_polygon_mode(int mode);
	void set_culling_mode(int mode);
	void set_shader(Shader shader);
	void update();
	VkPipeline selected();
};

std::unique_ptr<PipelineMatrixManager> createPipelineManager(INIReader renderer_reader);