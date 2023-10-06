/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */
#include "PathUtils.h"
#include "Utils.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vector>

#undef min
#undef max

#pragma region forward_declarations

/* --------------------------------------------- */
// Helper Function Declarations
/* --------------------------------------------- */

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices		A pointer which points to contiguous memory of #physical_device_count sequentially
                                        stored VkPhysicalDevice handles is expected. The handles can (or should) be those
 *										that are returned from vkEnumeratePhysicalDevices.
 *	@param		physical_device_count	The number of consecutive physical device handles there are at the memory location
 *										that is pointed to by the physical_devices parameter.
 *	@param		surface					A valid VkSurfaceKHR handle, which is used to determine if a certain
 *										physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const VkPhysicalDevice *physical_devices, uint32_t physical_device_count, VkSurfaceKHR surface);

/*!
 *	From the given list of physical devices, select the first one that satisfies all requirements.
 *	@param		physical_devices	A vector containing all available VkPhysicalDevice handles, like those
 *									that are returned from vkEnumeratePhysicalDevices.
 *	@param		surface				A valid VkSurfaceKHR handle, which is used to determine if a certain
 *									physical device supports presenting images to the given surface.
 *	@return		The index of the physical device that satisfies all requirements is returned.
 */
uint32_t selectPhysicalDeviceIndex(const std::vector<VkPhysicalDevice> &physical_devices, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, select a queue family which supports both,
 *	graphics and presentation to the given surface. Return the INDEX of an appropriate queue family!
 *	@return		The index of a queue family which supports the required features shall be returned.
 */
uint32_t selectQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a the physical device's surface capabilites are read and returned.
 *	@return		VkSurfaceCapabilitiesKHR data
 */
VkSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, a supported surface image format
 *	which can be used for the framebuffer's attachment formats is searched and returned.
 *	@return		A supported format is returned.
 */
VkSurfaceFormatKHR getSurfaceImageFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	Based on the given physical device and the surface, return its surface transform flag.
 *	This can be used to set the swap chain to the same configuration as the surface's current transform.
 *	@return		The surface capabilities' currentTransform value is returned, which is suitable for swap chain config.
 */
VkSurfaceTransformFlagBitsKHR getSurfaceTransform(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

/*!
 *	This callback function gets invoked by GLFW whenever a GLFW error occured.
 */
void errorCallbackFromGlfw(int error, const char *description) { std::cout << "GLFW error " << error << ": " << description << std::endl; }

#pragma endregion

#pragma region hide_this_stuff
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
    VkApplicationInfo application_info = {};                     // Zero-initialize every member
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Set this struct instance's type
    application_info.pEngineName = "GCG_VK_Library";             // Set some properties...
    application_info.engineVersion = VK_MAKE_API_VERSION(0, 2023, 9, 1);
    application_info.pApplicationName = "GCG_VK_Solution";
    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 2023, 9, 19);
    application_info.apiVersion = VK_API_VERSION_1_1; // Your system needs to support this Vulkan API version.

    VkInstanceCreateInfo instance_create_info = {};                      // Zero-initialize every member
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Set the struct's type
    instance_create_info.pApplicationInfo = &application_info;

    uint32_t required_glfw_extensions_count, required_vulkan_extension_count;
    const char **required_glfw_extensions = glfwGetRequiredInstanceExtensions(&required_glfw_extensions_count);
    const char **required_vulkan_extensions = vklGetRequiredInstanceExtensions(&required_vulkan_extension_count);

    const char *enabled_layers[1] = {"VK_LAYER_KHRONOS_validation"};

    std::vector<const char *> required_extensions;
    required_extensions.insert(required_extensions.end(), required_glfw_extensions, required_glfw_extensions + required_glfw_extensions_count);
    required_extensions.insert(required_extensions.end(), required_vulkan_extensions, required_vulkan_extensions + required_vulkan_extension_count);

    instance_create_info.enabledExtensionCount = required_extensions.size();
    instance_create_info.ppEnabledExtensionNames = &required_extensions.front();
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = enabled_layers;

    VkInstance vkInstance = VK_NULL_HANDLE;
    VkResult error = vkCreateInstance(&instance_create_info, nullptr, &vkInstance);
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
    uint32_t physical_device_count;
    VkResult error = vkEnumeratePhysicalDevices(vkInstance, &physical_device_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(error);

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    error = vkEnumeratePhysicalDevices(vkInstance, &physical_device_count, &physical_devices.front());
    VKL_CHECK_VULKAN_ERROR(error);

    uint32_t index = selectPhysicalDeviceIndex(physical_devices, vkSurface);
    return physical_devices[index];
}

VkDevice createVkDevice(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily)
{
    float queue_priority = 1.0f;
    const char *required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = &required_device_extensions;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;

    VkDevice vkDevice = VK_NULL_HANDLE;
    VkResult error = vkCreateDevice(vkPhysicalDevice, &device_create_info, nullptr, &vkDevice);
    VKL_CHECK_VULKAN_ERROR(error);
    return vkDevice;
}

struct VkDetailedImage
{
    VkImage image;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkExtent2D extent;
};

VkSwapchainKHR createVkSwapchain(VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkSurfaceKHR vkSurface, VkSurfaceFormatKHR vkSurfaceImageFormat, GLFWwindow *window, uint32_t queueFamily, std::vector<VkDetailedImage> &images)
{
    std::vector<uint32_t> queueFamilyIndices({queueFamily});
    VkSurfaceCapabilitiesKHR surface_capabilities = getPhysicalDeviceSurfaceCapabilities(vkPhysicalDevice, vkSurface);
    // Build the swapchain config struct:
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vkSurface;
    swapchain_create_info.minImageCount = surface_capabilities.minImageCount;
    swapchain_create_info.imageArrayLayers = 1u;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    else
    {
        std::cout << "Warning: Automatic Testing might fail, VK_IMAGE_USAGE_TRANSFER_SRC_BIT image usage is not supported" << std::endl;
    }
    swapchain_create_info.preTransform = surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1u;
    swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices.data();

    int clientWidth = 0, clientHeight = 0;
    glfwGetFramebufferSize(window, &clientWidth, &clientHeight);

    swapchain_create_info.imageFormat = vkSurfaceImageFormat.format;
    swapchain_create_info.imageColorSpace = vkSurfaceImageFormat.colorSpace;
    swapchain_create_info.imageExtent = {(uint32_t)clientWidth, (uint32_t)clientHeight};
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
    VkResult error = vkCreateSwapchainKHR(vkDevice, &swapchain_create_info, nullptr, &vkSwapchain);
    VKL_CHECK_VULKAN_ERROR(error);

    uint32_t swapchainImageCount = 0u;
    error = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &swapchainImageCount, nullptr);
    VKL_CHECK_VULKAN_ERROR(error);
    std::vector<VkImage> swapchainImages(swapchainImageCount);
    error = vkGetSwapchainImagesKHR(vkDevice, vkSwapchain, &swapchainImageCount, &swapchainImages.front());
    VKL_CHECK_VULKAN_ERROR(error);

    if (swapchainImageCount != surface_capabilities.minImageCount)
    {
        VKL_LOG("Swapchain image count does NOT match! " << std::to_string(swapchainImageCount) << " != " << std::to_string(surface_capabilities.minImageCount));
    }

    images.reserve(swapchainImages.size());
    for (const auto &image : swapchainImages)
    {
        images.push_back({
            .image = image,
            .format = swapchain_create_info.imageFormat,
            .colorSpace = swapchain_create_info.imageColorSpace,
            .extent = {(uint32_t)clientWidth, (uint32_t)clientHeight},
        });
    }

    return vkSwapchain;
}

VklSwapchainConfig createVklSwapchainConfig(VkSwapchainKHR vkSwapchain, std::vector<VkDetailedImage> &images)
{
    std::vector<VklSwapchainFramebufferComposition> swapchain_image_compositions;
    for (size_t i = 0; i < images.size(); i++)
    {
        VklSwapchainFramebufferComposition composition = {
            .colorAttachmentImageDetails = {
                .imageHandle = images[i].image,
                .imageFormat = images[i].format,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .clearValue = {0.8, 1.0, 1.0, 1.0},
            },
            .depthAttachmentImageDetails = {},
        };
        swapchain_image_compositions.push_back(composition);
    }

    // Gather swapchain config as required by the framework:
    return {
        .swapchainHandle = vkSwapchain,
        .imageExtent = images[0].extent,
        .swapchainImages = swapchain_image_compositions,
    };
}

#pragma endregion

struct Vertex
{
    glm::vec3 position;
};

struct FragmentUniformBlock
{
    glm::vec4 color;
};

/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char **argv)
{
    VKL_LOG(":::::: WELCOME TO GCG 2023 ::::::");

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

    GLFWwindow *window = createGLFWWindow();

    if (!window)
    {
        VKL_EXIT_WITH_ERROR("No GLFW window created.");
    }

    VkInstance vk_instance = createVkInstance();
    VkSurfaceKHR vk_surface = createVkSurface(vk_instance, window);
    VkPhysicalDevice vk_physical_device = createVkPhysicalDevice(vk_instance, vk_surface);
    uint32_t graphics_queue_family = selectQueueFamilyIndex(vk_physical_device, vk_surface);
    VkDevice vk_device = createVkDevice(vk_physical_device, graphics_queue_family);
    VkQueue vk_queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(vk_device, graphics_queue_family, 0, &vk_queue);
    std::vector<VkDetailedImage> swapchain_images;
    VkSurfaceFormatKHR vk_surface_image_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    VkSwapchainKHR vk_swapchain = createVkSwapchain(vk_physical_device, vk_device, vk_surface, vk_surface_image_format, window, graphics_queue_family, swapchain_images);

#pragma region check_instances
    if (!vk_instance)
    {
        VKL_EXIT_WITH_ERROR("No VkInstance created or handle not assigned.");
    }
    if (!vk_surface)
    {
        VKL_EXIT_WITH_ERROR("No VkSurfaceKHR created or handle not assigned.");
    }
    if (!vk_physical_device)
    {
        VKL_EXIT_WITH_ERROR("No VkPhysicalDevice selected or handle not assigned.");
    }
    if (!vk_device)
    {
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");
    }
    if (!vk_queue)
    {
        VKL_EXIT_WITH_ERROR("No VkQueue selected or handle not assigned.");
    }
    if (!vk_swapchain)
    {
        VKL_EXIT_WITH_ERROR("No VkSwapchainKHR created or handle not assigned.");
    }
#pragma endregion

    // Gather swapchain config as required by the framework:
    VklSwapchainConfig swapchain_config = createVklSwapchainConfig(vk_swapchain, swapchain_images);

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config))
    {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }

    VklGraphicsPipelineConfig graphics_pipeline_config = {
        .vertexShaderPath = "assets/shaders_vk/task2.vert",
        .fragmentShaderPath = "assets/shaders_vk/task2.frag",
        .vertexInputBuffers = {{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }},
        .inputAttributeDescriptions = {{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0u,
        }},
        .polygonDrawMode = VK_POLYGON_MODE_FILL,
        .triangleCullingMode = VK_CULL_MODE_NONE,
        .descriptorLayout = {{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        }},
    };
    VkPipeline vk_pipeline = vklCreateGraphicsPipeline(graphics_pipeline_config);
    VkBuffer fragment_uniform_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(FragmentUniformBlock), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    FragmentUniformBlock fragment_uniform_data = {
        .color = {1.0, 0.5, 0.0, 1.0},
    };
    vklCopyDataIntoHostCoherentBuffer(fragment_uniform_buffer, &fragment_uniform_data, sizeof(FragmentUniformBlock));

    VkDescriptorPoolSize descriptor_pool_size = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 8,
    };
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 8,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptor_pool_size,
    };
    VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;
    VkResult error = vkCreateDescriptorPool(vk_device, &descriptor_pool_create_info, nullptr, &vk_descriptor_pool);
    VKL_CHECK_VULKAN_ERROR(error);

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &descriptor_set_layout_binding,
    };
    VkDescriptorSetLayout vk_descriptor_set_layout = VK_NULL_HANDLE;
    error = vkCreateDescriptorSetLayout(vk_device, &descriptor_set_layout_create_info, nullptr, &vk_descriptor_set_layout);
    VKL_CHECK_VULKAN_ERROR(error);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vk_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vk_descriptor_set_layout,
    };
    VkDescriptorSet vk_descriptor_set = VK_NULL_HANDLE;
    error = vkAllocateDescriptorSets(vk_device, &descriptor_set_allocate_info, &vk_descriptor_set);
    VKL_CHECK_VULKAN_ERROR(error);

    VkDescriptorBufferInfo fragment_uniform_buffer_info = {
        .buffer = fragment_uniform_buffer,
        .offset = 0,
        .range = sizeof(FragmentUniformBlock),
    };
    VkWriteDescriptorSet vk_write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vk_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &fragment_uniform_buffer_info,
    };
    vkUpdateDescriptorSets(vk_device, 1, &vk_write_descriptor_set, 0, nullptr);

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } });
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();
        gcgDrawTeapot(vk_pipeline, vk_descriptor_set);
        vklEndRecordingCommands();
        vklPresentCurrentSwapchainImage();

        if (cmdline_args.run_headless)
        {
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename)
            {
                screenshot_filename = cmdline_args.filename;
            }
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            gcgSaveScreenshot(screenshot_filename, swapchain_images[idx].image, width,
                              height, vk_surface_image_format.format, vk_device, vk_physical_device, vk_queue,
                              graphics_queue_family);
            break;
        }
    }

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);

    vkDestroyDescriptorSetLayout(vk_device, vk_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, nullptr);
    vklDestroyHostCoherentBufferAndItsBackingMemory(fragment_uniform_buffer);
    vklDestroyGraphicsPipeline(vk_pipeline);
    gcgDestroyFramework();
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
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

                if (VK_TRUE == presentation_supported)
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