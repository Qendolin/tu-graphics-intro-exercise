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
	VkImageView view;
	VkFormat format;
	VkExtent2D extent;

public:
	Texture(VkImage image, VkFormat format, VkExtent2D extent, VkImageView view);
	void destroy(VkDevice device);
	void init_uniforms(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, VkSampler sampler);
};

std::vector<std::shared_ptr<Texture>> createTextureImages(VkDevice vk_device, VkQueue vk_queue, uint32_t queue_family, std::vector<std::string> names);
std::shared_ptr<Texture> createTextureCubeMap(VkDevice vk_device, VkQueue vk_queue, uint32_t queue_family, const std::array<std::string, 6> &names);
VkSampler createSampler(VkDevice vk_device, VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode);