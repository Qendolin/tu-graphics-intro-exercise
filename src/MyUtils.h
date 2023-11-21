#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include <memory>

class ITrash
{
public:
	virtual ~ITrash() {}
	virtual void destroy() = 0;
};

struct UniformBufferSlot
{
	VkDeviceSize offset;
	VkDeviceSize size;
};

class SharedUniformBuffer : public ITrash
{
private:
	VkDeviceSize element_size = 0;
	VkDeviceSize element_stride = 0;

public:
	VkBuffer buffer = VK_NULL_HANDLE;

	SharedUniformBuffer(VkPhysicalDevice device, VkDeviceSize element_size, uint32_t element_count);

	UniformBufferSlot slot(uint32_t index);
	void destroy();
};

struct VkDetailedImage
{
	VkImage image;
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkExtent2D extent;
};