#pragma once

#include <vulkan/vulkan.h>

inline PFN_vkCmdPipelineBarrier2KHR __vkCmdPipelineBarrier2KHR;
#define vkCmdPipelineBarrier2KHR __vkCmdPipelineBarrier2KHR

static void load_vulkan_extensions(VkDevice vk_device)
{
	__vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(vk_device, "vkCmdPipelineBarrier2KHR"));
}