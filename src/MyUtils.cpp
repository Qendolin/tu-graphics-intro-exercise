#include "MyUtils.h"

#include <VulkanLaunchpad.h>

#pragma region SharedUniformBuffer

SharedUniformBuffer::SharedUniformBuffer(VkPhysicalDevice device, VkDeviceSize element_size, uint32_t element_count)
{
	VkPhysicalDeviceProperties device_props = {};
	vkGetPhysicalDeviceProperties(device, &device_props);
	auto alignment = device_props.limits.minUniformBufferOffsetAlignment;
	// from https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
	this->element_stride = (element_size + alignment - 1) & ~(alignment - 1);
	this->element_size = element_size;
	this->buffer = vklCreateHostCoherentBufferWithBackingMemory(element_count * element_stride, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

UniformBufferSlot SharedUniformBuffer::slot(uint32_t index)
{
	return UniformBufferSlot{element_stride * index, element_size};
}

void SharedUniformBuffer::destroy(VkDevice device)
{
	vklDestroyHostCoherentBufferAndItsBackingMemory(buffer);
}

#pragma endregion