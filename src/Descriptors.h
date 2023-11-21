#pragma once

#include <vulkan/vulkan.h>
#include <VulkanLaunchpad.h>

#include "MyUtils.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>

VkDescriptorPool createVkDescriptorPool(VkDevice vkDevice, uint32_t maxSets, uint32_t descriptorCount);

struct DescriptorSetLayoutParams
{
	uint32_t binding;
	VkDescriptorType type;
};

VkDescriptorSetLayout createVkDescriptorSetLayout(VkDevice vkDevice, std::vector<DescriptorSetLayoutParams> params);
VkDescriptorSet createVkDescriptorSet(VkDevice vkDevice, VkDescriptorPool vkDescriptorPool, VkDescriptorSetLayout vkDescriptorSetLayout);
void writeDescriptorSetBuffer(VkDevice vkDevice, VkDescriptorSet dst, uint32_t binding, VkBuffer buffer, size_t size, UniformBufferSlot range = {0, (VkDeviceSize)-1});