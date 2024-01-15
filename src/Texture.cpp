#include "Texture.h"
#include "vulkan_ext.h"
#include "Descriptors.h"
#include "PathUtils.h"
#include <glm/glm.hpp>

// Why is max defined as a macro?
#undef max

#pragma region Texture
Texture::Texture(VkImage image, VkFormat format, VkExtent2D extent, VkImageView view)
{
	this->image = image;
	this->format = format;
	this->extent = extent;
	this->view = view;
}

void Texture::init_uniforms(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding, VkSampler sampler)
{
	writeDescriptorSetImage(device, descriptor_set, binding, sampler, this->view);
}

void Texture::destroy(VkDevice device)
{
	vklDestroyDeviceLocalImageAndItsBackingMemory(this->image);
	vkDestroyImageView(device, this->view, nullptr);
}
#pragma endregion

VkImage loadImageToTexture(VkCommandBuffer vk_cmd_buf, std::vector<VklImageInfo> level_infos, std::vector<VkBuffer> level_host_bufs)
{
	VkImage vk_img = vklCreateDeviceLocalImageWithBackingMemory(level_infos[0].extent.width, level_infos[0].extent.height, level_infos[0].imageFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	uint32_t levelCount = level_infos.size();

	VkImageMemoryBarrier2 vk_img_barrier_first = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
		.dstAccessMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vk_img,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};
	VkDependencyInfo vk_img_dep_info_first = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.dependencyFlags = 0,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &vk_img_barrier_first,
	};
	vkCmdPipelineBarrier2KHR(vk_cmd_buf, &vk_img_dep_info_first);

	for (uint32_t i = 0; i < levelCount; i++)
	{
		VkBufferImageCopy vk_img_copy_region = {
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = i,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageExtent = {.width = level_infos[i].extent.width, .height = level_infos[i].extent.height, .depth = 1},
		};
		vkCmdCopyBufferToImage(vk_cmd_buf, level_host_bufs[i], vk_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_img_copy_region);
	}

	VkImageMemoryBarrier2 vk_img_barrier_second = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vk_img,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};
	VkDependencyInfo vk_img_dep_info_second = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.dependencyFlags = 0,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &vk_img_barrier_second,
	};
	vkCmdPipelineBarrier2KHR(vk_cmd_buf, &vk_img_dep_info_second);

	return vk_img;
}

std::vector<std::shared_ptr<Texture>> createTextureImages(VkDevice vk_device, VkQueue vk_queue, uint32_t queue_family, std::vector<std::string> names)
{
	VkCommandPool vk_img_cmd_pool = VK_NULL_HANDLE;
	VkCommandPoolCreateInfo vk_img_cmd_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = queue_family,
	};
	VkResult error = vkCreateCommandPool(vk_device, &vk_img_cmd_pool_create_info, nullptr, &vk_img_cmd_pool);
	VKL_CHECK_VULKAN_ERROR(error);

	VkCommandBuffer vk_img_cmd_buf = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo vk_img_cmd_buf_alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = vk_img_cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	error = vkAllocateCommandBuffers(vk_device, &vk_img_cmd_buf_alloc_info, &vk_img_cmd_buf);
	VKL_CHECK_VULKAN_ERROR(error);

	VkCommandBufferBeginInfo vk_img_cmd_buf_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
	};
	error = vkBeginCommandBuffer(vk_img_cmd_buf, &vk_img_cmd_buf_begin_info);
	VKL_CHECK_VULKAN_ERROR(error);

	std::vector<VkBuffer> host_buffers;
	std::vector<std::shared_ptr<Texture>> result;
	for (auto &&name : names)
	{
		std::string path = gcgFindTextureFile("assets/textures/" + name);
		VklImageInfo img_info = vklGetDdsImageInfo(path.c_str());

		uint32_t mipLevels = static_cast<uint32_t>(1 + std::floor(std::log2(std::max(img_info.extent.width, img_info.extent.height))));
		std::vector<VkBuffer> level_bufs(mipLevels);
		std::vector<VklImageInfo> level_infos(mipLevels);
		for (size_t i = 0; i < mipLevels; i++)
		{
			level_infos[i] = vklGetDdsImageLevelInfo(path.c_str(), i);
			level_bufs[i] = vklLoadDdsImageLevelIntoHostCoherentBuffer(path.c_str(), i);
			host_buffers.push_back(level_bufs[i]);
		}
		VkImage image = loadImageToTexture(vk_img_cmd_buf, level_infos, level_bufs);

		VkImageView image_view = VK_NULL_HANDLE;
		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.flags = 0,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = img_info.imageFormat,
			.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		error = vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view);
		VKL_CHECK_VULKAN_ERROR(error);

		result.push_back(std::make_shared<Texture>(image, img_info.imageFormat, img_info.extent, image_view));
	}
	error = vkEndCommandBuffer(vk_img_cmd_buf);
	VKL_CHECK_VULKAN_ERROR(error);

	VkFence vk_img_fence = VK_NULL_HANDLE;
	VkFenceCreateInfo vk_img_fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0,
	};
	error = vkCreateFence(vk_device, &vk_img_fence_create_info, nullptr, &vk_img_fence);
	VKL_CHECK_VULKAN_ERROR(error);

	VkSubmitInfo vk_img_submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_img_cmd_buf,
	};

	error = vkQueueSubmit(vk_queue, 1, &vk_img_submit_info, vk_img_fence);
	VKL_CHECK_VULKAN_ERROR(error);

	error = vkWaitForFences(vk_device, 1, &vk_img_fence, VK_TRUE, UINT64_MAX);
	VKL_CHECK_VULKAN_ERROR(error);

	vkDestroyCommandPool(vk_device, vk_img_cmd_pool, nullptr);
	vkDestroyFence(vk_device, vk_img_fence, nullptr);
	for (auto &&buf : host_buffers)
	{
		vklDestroyHostCoherentBufferAndItsBackingMemory(buf);
	}
	return result;
}

VkSampler createSampler(VkDevice vk_device, VkFilter minFilter, VkFilter magFilter, VkSamplerMipmapMode mipmapMode)
{
	VkSampler sampler = VK_NULL_HANDLE;
	VkSamplerCreateInfo sampler_create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.flags = 0,
		.magFilter = minFilter,
		.minFilter = magFilter,
		.mipmapMode = mipmapMode,
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
	};
	VkResult error = vkCreateSampler(vk_device, &sampler_create_info, nullptr, &sampler);
	VKL_CHECK_VULKAN_ERROR(error);
	return sampler;
}