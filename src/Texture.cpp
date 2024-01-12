#include "Texture.h"
#include "vulkan_ext.h"

#pragma region Texture
Texture::Texture(VkImage image, VkFormat format, VkExtent2D extent, VkImageView view, VkSampler sampler)
{
	this->image = image;
	this->format = format;
	this->extent = extent;
	this->view = view;
	this->sampler = sampler;
}

void Texture::destroy(VkDevice device)
{
	vklDestroyDeviceLocalImageAndItsBackingMemory(this->image);
	vkDestroyImageView(device, this->view, nullptr);
	vkDestroySampler(device, this->sampler, nullptr);
}
#pragma endregion

VkImage loadImageToTexture(VkCommandBuffer vk_cmd_buf, uint32_t queue_family, VklImageInfo img_info, VkBuffer img_host_buf)
{
	VkImage vk_img = vklCreateDeviceLocalImageWithBackingMemory(img_info.extent.width, img_info.extent.height, img_info.imageFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	VkImageMemoryBarrier2 vk_img_barrier_first = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = queue_family,
		.dstQueueFamilyIndex = queue_family,
		.image = vk_img,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
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

	VkBufferImageCopy vk_img_copy_region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageExtent = {.width = img_info.extent.width, .height = img_info.extent.height, .depth = 1},
	};
	vkCmdCopyBufferToImage(vk_cmd_buf, img_host_buf, vk_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vk_img_copy_region);

	VkImageMemoryBarrier2 vk_img_barrier_second = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = queue_family,
		.dstQueueFamilyIndex = queue_family,
		.image = vk_img,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
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
		std::string path = "assets/textures/" + name;
		VklImageInfo img_info = vklGetDdsImageInfo(path.c_str());
		VkBuffer img_host_buf = vklLoadDdsImageIntoHostCoherentBuffer(path.c_str());
		host_buffers.push_back(img_host_buf);
		VkImage image = loadImageToTexture(vk_img_cmd_buf, queue_family, img_info, img_host_buf);
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
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		error = vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view);
		VKL_CHECK_VULKAN_ERROR(error);

		VkSampler image_sampler = VK_NULL_HANDLE;
		VkSamplerCreateInfo image_sampler_create_info = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.minLod = 0.0f,
			.maxLod = VK_LOD_CLAMP_NONE,
		};
		error = vkCreateSampler(vk_device, &image_sampler_create_info, nullptr, &image_sampler);
		VKL_CHECK_VULKAN_ERROR(error);

		result.push_back(std::make_shared<Texture>(image, img_info.imageFormat, img_info.extent, image_view, image_sampler));
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