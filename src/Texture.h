#pragma once

#include <vulkan/vulkan.h>
#include <VulkanLaunchpad.h>
#include <GLFW/glfw3.h>

#include "MyUtils.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>

class Texture : public ITrash
{
private:
	VkImage image;
	VkFormat format;
	VkExtent2D extent;

public:
	Texture(VkImage image, VkFormat format, VkExtent2D extent);
	void destroy();
};

std::vector<std::shared_ptr<Texture>> createTextureImages(VkDevice vk_device, VkQueue vk_queue, uint32_t queue_family, std::vector<std::string> names);