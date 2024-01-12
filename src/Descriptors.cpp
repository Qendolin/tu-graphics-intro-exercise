#include "Descriptors.h"

VkDescriptorPool createVkDescriptorPool(VkDevice vkDevice, uint32_t maxSets, uint32_t descriptorCount)
{
	VkDescriptorPoolSize descriptorPoolSize = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = descriptorCount,
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxSets,
		.poolSizeCount = 1,
		.pPoolSizes = &descriptorPoolSize,
	};
	VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
	VkResult error = vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, nullptr, &vkDescriptorPool);
	VKL_CHECK_VULKAN_ERROR(error);

	return vkDescriptorPool;
}

VkDescriptorSetLayout createVkDescriptorSetLayout(VkDevice vkDevice, std::vector<DescriptorSetLayoutParams> params)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(params.size());
	for (auto &param : params)
	{
		bindings.push_back({
			.binding = param.binding,
			.descriptorType = param.type,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
		});
	}
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = uint32_t(bindings.size()),
		.pBindings = &bindings.front(),
	};
	VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
	VkResult error = vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
	VKL_CHECK_VULKAN_ERROR(error);

	return vkDescriptorSetLayout;
}

VkDescriptorSet createVkDescriptorSet(VkDevice vkDevice, VkDescriptorPool vkDescriptorPool, VkDescriptorSetLayout vkDescriptorSetLayout)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = vkDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vkDescriptorSetLayout,
	};
	VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
	VkResult error = vkAllocateDescriptorSets(vkDevice, &descriptorSetAllocateInfo, &vkDescriptorSet);
	VKL_CHECK_VULKAN_ERROR(error);

	return vkDescriptorSet;
}

void writeDescriptorSetBuffer(VkDevice vkDevice, VkDescriptorSet dst, uint32_t binding, VkBuffer buffer, size_t size, UniformBufferSlot range)
{
	VkDescriptorBufferInfo bufferInfo = {
		.buffer = buffer,
		.offset = range.offset,
		.range = range.size == (VkDeviceSize)-1 ? VK_WHOLE_SIZE : range.size,
	};
	VkWriteDescriptorSet vkWriteDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dst,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferInfo,
	};
	vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
}

void writeDescriptorSetImage(VkDevice vkDevice, VkDescriptorSet dst, uint32_t binding, VkSampler sampler, VkImageView view)
{
	VkDescriptorImageInfo imageInfo = {
		.sampler = sampler,
		.imageView = view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	VkWriteDescriptorSet vkWriteDescriptorSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = dst,
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo,
	};
	std::cout << "writeDescriptorSetImage::vkUpdateDescriptorSets, sampler=" << sampler << ", view=" << view << std::endl
			  << std::flush;
	vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, nullptr);
}