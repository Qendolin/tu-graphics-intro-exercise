#include "Setup.h"

#include "INIReader.h"

void errorCallbackFromGlfw(int error, const char *description) { std::cout << "GLFW error " << error << ": " << description << std::endl; }

GLFWwindow *createGLFWWindow()
{
	INIReader window_reader("assets/settings/window.ini");
	int window_width = window_reader.GetInteger("window", "width", 800);
	int window_height = window_reader.GetInteger("window", "height", 800);
	std::string window_title = window_reader.Get("window", "title", "GCG 2023");
	bool fullscreen = false;
	// Install a callback function, which gets invoked whenever a GLFW error occurred.
	glfwSetErrorCallback(errorCallbackFromGlfw);

	if (!glfwInit())
	{
		VKL_EXIT_WITH_ERROR("Failed to init GLFW");
	}

	if (!glfwVulkanSupported())
	{
		VKL_EXIT_WITH_ERROR("Vulkan is not supported");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No need to create a graphics context for Vulkan
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Use a monitor if we'd like to open the window in fullscreen mode:
	GLFWmonitor *monitor = nullptr;
	if (fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
	}

	return glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);
}

VkInstance createVkInstance()
{
	VkApplicationInfo application_info = {};					 // Zero-initialize every member
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Set this struct instance's type
	application_info.pEngineName = "GCG_VK_Library";			 // Set some properties...
	application_info.engineVersion = VK_MAKE_API_VERSION(0, 2023, 9, 1);
	application_info.pApplicationName = "GCG_VK_Solution";
	application_info.applicationVersion = VK_MAKE_API_VERSION(0, 2023, 9, 19);
	application_info.apiVersion = VK_API_VERSION_1_1; // Your system needs to support this Vulkan API version.

	VkInstanceCreateInfo instanceCreateInfo = {};					   // Zero-initialize every member
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Set the struct's type
	instanceCreateInfo.pApplicationInfo = &application_info;

	uint32_t required_glfw_extensions_count, required_vulkan_extension_count;
	const char **required_glfw_extensions = glfwGetRequiredInstanceExtensions(&required_glfw_extensions_count);
	const char **required_vulkan_extensions = vklGetRequiredInstanceExtensions(&required_vulkan_extension_count);

	const char *enabled_layers[1] = {"VK_LAYER_KHRONOS_validation"};

	std::vector<const char *> required_extensions;
	required_extensions.insert(required_extensions.end(), required_glfw_extensions, required_glfw_extensions + required_glfw_extensions_count);
	required_extensions.insert(required_extensions.end(), required_vulkan_extensions, required_vulkan_extensions + required_vulkan_extension_count);

	instanceCreateInfo.enabledExtensionCount = required_extensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = &required_extensions.front();
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = enabled_layers;

	VkInstance vkInstance = VK_NULL_HANDLE;
	VkResult error = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance);
	VKL_CHECK_VULKAN_ERROR(error);
	return vkInstance;
}

VkSurfaceKHR createVkSurface(VkInstance vkInstance, GLFWwindow *window)
{
	VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
	VkResult error = glfwCreateWindowSurface(vkInstance, window, nullptr, &vkSurface);
	VKL_CHECK_VULKAN_ERROR(error);
	return vkSurface;
}

VkPhysicalDevice createVkPhysicalDevice(VkInstance vkInstance, VkSurfaceKHR vkSurface)
{
	VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
	uint32_t physicalDeviceCount;
	VkResult error = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
	VKL_CHECK_VULKAN_ERROR(error);

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	error = vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, &physicalDevices.front());
	VKL_CHECK_VULKAN_ERROR(error);

	uint32_t index = selectPhysicalDeviceIndex(physicalDevices, vkSurface);
	return physicalDevices[index];
}

VkDevice createVkDevice(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily)
{
	float queuePriority = 1.0f;
	std::vector<const char *> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};
	const VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};
	const VkPhysicalDeviceFeatures deviceFeatures = {
		.fillModeNonSolid = VK_TRUE,
	};
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = &requiredDeviceExtensions.front();
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	const VkPhysicalDeviceSynchronization2Features vk_sync_feature = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.synchronization2 = VK_TRUE,
	};
	deviceCreateInfo.pNext = &vk_sync_feature;

	VkDevice vkDevice = VK_NULL_HANDLE;
	VkResult error = vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice);
	VKL_CHECK_VULKAN_ERROR(error);
	return vkDevice;
}

VkSwapchainKHR createVkSwapchain(VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkSurfaceKHR vkSurface, VkSurfaceFormatKHR vkSurfaceImageFormat, GLFWwindow *window, uint32_t queueFamily, std::vector<VkDetailedImage> &colorAttachments, VkDetailedImage *depthAttachment)
{
	std::vector<uint32_t> queueFamilyIndices({queueFamily});
	VkSurfaceCapabilitiesKHR surfaceCapabilities = getPhysicalDeviceSurfaceCapabilities(vkPhysicalDevice, vkSurface);
	// Build the swapchain config struct:
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = vkSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageArrayLayers = 1u;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	else
	{
		std::cout << "Warning: Automatic Testing might fail, VK_IMAGE_USAGE_TRANSFER_SRC_BIT image usage is not supported" << std::endl;
	}
	swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 1u;
	swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	int viewportWidth = 0, viewportHeight = 0;
	glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);

	swapchainCreateInfo.imageFormat = vkSurfaceImageFormat.format;
	swapchainCreateInfo.imageColorSpace = vkSurfaceImageFormat.colorSpace;
	swapchainCreateInfo.imageExtent = {(uint32_t)viewportWidth, (uint32_t)viewportHeight};
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
	VkResult error = vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &vkSwapchain);
	VKL_CHECK_VULKAN_ERROR(error);

	uint32_t swapchainImageCount = 0u;
	error = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &swapchainImageCount, nullptr);
	VKL_CHECK_VULKAN_ERROR(error);
	std::vector<VkImage> swapchainImages(swapchainImageCount);
	error = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &swapchainImageCount, &swapchainImages.front());
	VKL_CHECK_VULKAN_ERROR(error);

	if (swapchainImageCount != surfaceCapabilities.minImageCount)
	{
		VKL_LOG("Swapchain image count does NOT match! " << std::to_string(swapchainImageCount) << " != " << std::to_string(surfaceCapabilities.minImageCount));
	}

	colorAttachments.reserve(swapchainImages.size());
	for (const auto &image : swapchainImages)
	{
		colorAttachments.push_back({
			.image = image,
			.format = swapchainCreateInfo.imageFormat,
			.colorSpace = swapchainCreateInfo.imageColorSpace,
			.extent = {(uint32_t)viewportWidth, (uint32_t)viewportHeight},
		});
	}

	VkImage depthAttachmentImage = vklCreateDeviceLocalImageWithBackingMemory(vkPhysicalDevice, vkDevice, viewportWidth, viewportHeight, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	*depthAttachment = {
		.image = depthAttachmentImage,
		.format = VK_FORMAT_D32_SFLOAT,
		.extent = {(uint32_t)viewportWidth, (uint32_t)viewportHeight},
	};

	return vkSwapchain;
}

VklSwapchainConfig createVklSwapchainConfig(VkSwapchainKHR vkSwapchain, std::vector<VkDetailedImage> &colorAttachments, VkDetailedImage &depthAttachment)
{
	std::vector<VklSwapchainFramebufferComposition> swapchainImageCompositions;
	for (size_t i = 0; i < colorAttachments.size(); i++)
	{
		VklSwapchainFramebufferComposition composition = {
			.colorAttachmentImageDetails = {
				.imageHandle = colorAttachments[i].image,
				.imageFormat = colorAttachments[i].format,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.clearValue = {0.8, 1.0, 1.0, 1.0},
			},
			.depthAttachmentImageDetails = {
				.imageHandle = depthAttachment.image,
				.imageFormat = depthAttachment.format,
				.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.clearValue = {.depthStencil = {1.0f, 0}},
			},
		};
		swapchainImageCompositions.push_back(composition);
	}

	// Gather swapchain config as required by the framework:
	return {
		.swapchainHandle = vkSwapchain,
		.imageExtent = colorAttachments[0].extent,
		.swapchainImages = swapchainImageCompositions,
	};
}

/* --------------------------------------------- */
// Helper Function Definitions
/* --------------------------------------------- */

#pragma region helper_functions
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice *physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface)
{
	// Iterate over all the physical devices and select one that satisfies all our requirements.
	// Our requirements are:
	//  - Must support a queue that must have both, graphics and presentation capabilities
	for (uint32_t physical_device_index = 0u; physical_device_index < physical_device_count; ++physical_device_index)
	{
		// Get the number of different queue families:
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, nullptr);

		// Get the queue families' data:
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[physical_device_index], &queue_family_count, queue_families.data());

		for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index)
		{
			// If this physical device supports a queue family which supports both, graphics and presentation
			//  => select this physical device
			if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				// This queue supports graphics! Let's see if it also supports presentation:
				VkBool32 presentation_supported;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[physical_device_index], queue_family_index, surface, &presentation_supported);
				VkPhysicalDeviceFeatures featues;
				vkGetPhysicalDeviceFeatures(physical_devices[physical_device_index], &featues);

				if (presentation_supported == VK_TRUE && featues.fillModeNonSolid == VK_TRUE)
				{
					// We've found a suitable physical device
					return physical_device_index;
				}
			}
		}
	}
	VKL_EXIT_WITH_ERROR("Unable to find a suitable physical device that supports graphics and presentation on the same queue.");
}

uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice> &physical_devices, VkSurfaceKHR surface)
{
	return selectPhysicalDeviceIndex(physical_devices.data(), static_cast<uint32_t>(physical_devices.size()), surface);
}

uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	// Get the number of different queue families for the given physical device:
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	// Get the queue families' data:
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	for (uint32_t queue_family_index = 0u; queue_family_index < queue_family_count; ++queue_family_index)
	{
		// If this physical device supports a queue family which supports both, graphics and presentation
		//  => select this physical device
		if ((queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			// This queue supports graphics! Let's see if it also supports presentation:
			VkBool32 presentation_supported;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, surface, &presentation_supported);

			if (VK_TRUE == presentation_supported)
			{
				// We've found a suitable physical device
				return queue_family_index;
			}
		}
	}

	VKL_EXIT_WITH_ERROR("Unable to find a suitable queue family that supports graphics and presentation on the same queue.");
}

VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	VkResult result;

	uint32_t surface_format_count;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
	VKL_CHECK_VULKAN_ERROR(result);

	std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats.data());
	VKL_CHECK_VULKAN_ERROR(result);

	if (surface_formats.empty())
	{
		VKL_EXIT_WITH_ERROR("Unable to find supported surface formats.");
	}

	// Prefer a RGB8/sRGB format; If we are unable to find such, just return any:
	for (const VkSurfaceFormatKHR &f : surface_formats)
	{
		if ((f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB) && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return f;
		}
	}

	return surface_formats[0];
}

VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
	VKL_CHECK_VULKAN_ERROR(result);
	return surface_capabilities;
}

VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	return getPhysicalDeviceSurfaceCapabilities(physical_device, surface).currentTransform;
}
#pragma endregion