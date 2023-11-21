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
#include <functional>
#include <algorithm>
#include <iterator>

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

class ITrash
{
public:
    virtual ~ITrash() {}
    virtual void destroy() = 0;
};

std::vector<std::shared_ptr<ITrash>> trash;

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

struct MeshInstanceUniformBlock
{
    glm::vec4 color;
    glm::mat4 modelMatrix;
};

struct CameraUniformBlock
{
    glm::mat4 viewProjectionMatrix;
};

#pragma region hide_this_stuff

std::vector<uint32_t> cube_indices = {
    // Top
    7, 6, 2,
    2, 3, 7,

    // Bottom
    0, 4, 5,
    5, 1, 0,

    // Left
    0, 2, 6,
    6, 4, 0,

    // Right
    7, 3, 1,
    1, 5, 7,

    // Front
    3, 2, 0,
    0, 1, 3,

    // Back
    4, 6, 7,
    7, 5, 4};

glm::vec3 cube_positions[]{
    {-0.5, -0.5, 0.5},  // 0
    {0.5, -0.5, 0.5},   // 1
    {-0.5, 0.5, 0.5},   // 2
    {0.5, 0.5, 0.5},    // 3
    {-0.5, -0.5, -0.5}, // 4
    {0.5, -0.5, -0.5},  // 5
    {-0.5, 0.5, -0.5},  // 6
    {0.5, 0.5, -0.5},   // 7
};

std::vector<Vertex> createCubeVertices(float width, float height, float depth, glm::vec3 color)
{
    std::vector<glm::vec3> positions(std::begin(cube_positions), std::end(cube_positions));

    auto scale = glm::mat4(1.0);
    scale = glm::scale(scale, {width, height, depth});

    for (size_t i = 0; i < positions.size(); i++)
    {
        positions[i] = glm::vec3(scale * glm::vec4(positions[i], 1.0f));
    }

    auto vertices = std::vector<Vertex>(positions.size());
    for (size_t i = 0; i < vertices.size(); i++)
    {
        vertices[i] = {positions[i], color};
    }

    return vertices;
}

uint32_t cornell_position_swizzle[]{
    // Top
    2, 6, 7, 3,
    // Bottom
    5, 4, 0, 1,
    // Left
    6, 2, 0, 4,
    // Right
    1, 3, 7, 5,
    // Back
    7, 6, 4, 5};

std::vector<uint32_t> cornell_indices = {
    // Top
    0, 1, 2,
    2, 3, 0,

    // Bottom
    4, 5, 6,
    6, 7, 4,

    // Left
    8, 9, 10,
    10, 11, 8,

    // Right
    12, 13, 14,
    14, 15, 12,

    // Back
    16, 17, 18,
    18, 19, 16};

glm::vec3 cornell_colors[]{
    {0.96, 0.93, 0.85}, // Top
    {0.64, 0.64, 0.64}, // Bottom
    {1.0, 0.0, 0.0},    // Left
    {0.0, 1.0, 0.0},    // Right
    {0.76, 0.74, 0.68}  // Back
};

std::vector<Vertex> createCornellVertices(float width, float height, float depth)
{
    std::vector<glm::vec3> positions(std::begin(cube_positions), std::end(cube_positions));

    auto scale = glm::mat4(1.0);
    scale = glm::scale(scale, {width, height, depth});

    for (size_t i = 0; i < positions.size(); i++)
    {
        positions[i] = glm::vec3(scale * glm::vec4(positions[i], 1.0f));
    }

    auto vertices = std::vector<Vertex>(5 * 4);
    for (size_t face = 0; face < 5; face++)
    {
        for (size_t v = 0; v < 4; v++)
        {
            size_t i = face * 4 + v;
            vertices[i].position = positions[cornell_position_swizzle[i]];
            vertices[i].color = cornell_colors[face];
        }
    }

    return vertices;
}

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

    VkInstanceCreateInfo instanceCreateInfo = {};                      // Zero-initialize every member
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
    const char *requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = &requiredDeviceExtensions;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkDevice vkDevice = VK_NULL_HANDLE;
    VkResult error = vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice);
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

struct DescriptorSetLayoutParams
{
    uint32_t binding;
    VkDescriptorType type;
};

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

    SharedUniformBuffer(VkPhysicalDevice device, VkDeviceSize element_size, uint32_t element_count)
    {
        VkPhysicalDeviceProperties device_props = {};
        vkGetPhysicalDeviceProperties(device, &device_props);
        auto alignment = device_props.limits.minUniformBufferOffsetAlignment;
        // from https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
        this->element_stride = (element_size + alignment - 1) & ~(alignment - 1);
        this->element_size = element_size;
        this->buffer = vklCreateHostCoherentBufferWithBackingMemory(element_count * element_stride, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    UniformBufferSlot slot(uint32_t index)
    {
        return UniformBufferSlot{element_stride * index, element_size};
    }

    virtual void destroy()
    {
        vklDestroyHostCoherentBufferAndItsBackingMemory(buffer);
    }
};

void writeDescriptorSetBuffer(VkDevice vkDevice, VkDescriptorSet dst, uint32_t binding, VkBuffer buffer, size_t size, UniformBufferSlot range = {0, (VkDeviceSize)-1})
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

class Camera : public ITrash
{
private:
    CameraUniformBlock uniform_block;
    VkBuffer uniform_buffer = VK_NULL_HANDLE;

public:
    float fovRad;
    glm::vec2 viewportSize;
    float nearPlane;
    float farPlane;
    glm::vec3 position;
    // pitch, yaw, roll
    glm::vec3 angles;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles)
    {
        this->fovRad = fovRad;
        this->viewportSize = viewportSize;
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;
        this->position = position;
        this->angles = angles;

        uniform_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(uniform_block), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        updateProjection();
        updateView();
    }

    void updateProjection()
    {
        float aspect = viewportSize.x / viewportSize.y;
        projectionMatrix = gcgCreatePerspectiveProjectionMatrix(fovRad, aspect, nearPlane, farPlane);
        set_uniforms({projectionMatrix * viewMatrix});
    }

    void updateView()
    {
        viewMatrix = glm::mat4(1.0f);
        viewMatrix = glm::translate(viewMatrix, position);
        viewMatrix = glm::rotate(viewMatrix, angles.z, {0, 0, 1});
        viewMatrix = glm::rotate(viewMatrix, angles.y, {0, 1, 0});
        viewMatrix = glm::rotate(viewMatrix, angles.x, {1, 0, 0});
        viewMatrix = glm::inverse(viewMatrix);
        set_uniforms({projectionMatrix * viewMatrix});
    }

    void init_uniforms(VkDevice device, VkDescriptorSet descriptor_set, uint32_t binding)
    {
        writeDescriptorSetBuffer(device, descriptor_set, binding, uniform_buffer, sizeof(uniform_block));
        set_uniforms(uniform_block);
    }

    void set_uniforms(CameraUniformBlock data)
    {
        uniform_block = data;
        if (uniform_buffer != VK_NULL_HANDLE)
            vklCopyDataIntoHostCoherentBuffer(uniform_buffer, &uniform_block, sizeof(uniform_block));
    }

    virtual void destroy()
    {
        vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer);
    }
};

std::unique_ptr<Camera> createCamera(std::string init_path, GLFWwindow *window)
{
    INIReader camera_ini_reader(init_path);
    double fovDeg = camera_ini_reader.GetReal("camera", "fov", 60);
    double nearPlane = camera_ini_reader.GetReal("camera", "near", 0.1);
    double farPlane = camera_ini_reader.GetReal("camera", "far", 100.0);
    double yaw = camera_ini_reader.GetReal("camera", "yaw", 0.0);
    double pitch = camera_ini_reader.GetReal("camera", "pitch", 0.0);

    int viewportWidth, viewportHeight;
    glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
    return std::make_unique<Camera>(glm::radians(fovDeg), glm::vec2(viewportWidth, viewportHeight), nearPlane, farPlane, glm::vec3(), glm::vec3(pitch, yaw, 0));
}

class Input
{
private:
    static std::shared_ptr<Input> _instance;

    float _prevTime;
    float _time;
    float _timeDelta;
    glm::vec2 _mousePrevPos;
    glm::vec2 _mousePos;
    glm::vec2 _mouseDelta;
    glm::vec2 _scrollDelta;
    glm::vec2 _scrollNextDelta;
    bool _mousePrevButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    bool _mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    bool _keysPrev[GLFW_KEY_LAST + 1];
    bool _keys[GLFW_KEY_LAST + 1];

public:
    const glm::vec2 mousePos() { return _mousePos; }
    const glm::vec2 mouseDelta() { return _mouseDelta; }
    const glm::vec2 scrollDelta() { return _scrollDelta; }
    const glm::vec2 timeDelta() { return _mouseDelta; }

    const bool isMouseDown(int button)
    {
        return _mouseButtons[button];
    }

    const bool isMouseTap(int button)
    {
        return _mouseButtons[button] && !_mousePrevButtons[button];
    }

    const bool isKeyDown(int key)
    {
        return _keys[key];
    }

    const bool isKeyTap(int key)
    {
        return _keys[key] && !_keysPrev[key];
    }

    void update()
    {
        _time = glfwGetTime();
        _timeDelta = _time - _prevTime;
        _mouseDelta = _mousePos - _mousePrevPos;
        _mousePrevPos = _mousePos;
        _scrollDelta = _scrollNextDelta;
        _scrollNextDelta = glm::vec2(0.0f);
        std::copy(std::begin(_mouseButtons), std::end(_mouseButtons), std::begin(_mousePrevButtons));
        std::copy(std::begin(_keys), std::end(_keys), std::begin(_keysPrev));
    }

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        _keys[key] = action != GLFW_RELEASE;
    }

    void onCursorPos(GLFWwindow *window, double x, double y)
    {
        _mousePos.x = x;
        _mousePos.y = y;
    }

    void onMouseButton(GLFWwindow *window, int button, int action, int mods)
    {
        _mouseButtons[button] = action != GLFW_RELEASE;
    }

    void onScroll(GLFWwindow *window, double dx, double dy)
    {
        _scrollNextDelta.x += dx;
        _scrollNextDelta.y += dy;
    }

    static std::shared_ptr<Input> instance();
    static std::shared_ptr<Input> init(GLFWwindow *window);
};

std::shared_ptr<Input> Input::_instance = nullptr;

std::shared_ptr<Input> Input::instance()
{
    return Input::_instance;
}

std::shared_ptr<Input> Input::init(GLFWwindow *window)
{
    Input::_instance = std::shared_ptr<Input>(new Input());
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       { Input::_instance->onKey(window, key, scancode, action, mods); });
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y)
                             { Input::_instance->onCursorPos(window, x, y); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods)
                               { Input::_instance->onMouseButton(window, button, action, mods); });
    glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy)
                          { Input::_instance->onScroll(window, dx, dy); });
    return Input::_instance;
}

struct PipelineParams
{
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullingMode;
};

VkPipeline createVkPipeline(PipelineParams &params)
{
    VklGraphicsPipelineConfig graphics_pipeline_config = {
        .vertexShaderPath = params.vertexShaderPath.c_str(),
        .fragmentShaderPath = params.fragmentShaderPath.c_str(),
        .vertexInputBuffers = {{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }},
        .inputAttributeDescriptions = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position),
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color),
            }},
        .polygonDrawMode = params.polygonMode,
        .triangleCullingMode = params.cullingMode,
        .descriptorLayout = {{
                                 .binding = 0,
                                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 .descriptorCount = 1,
                                 .stageFlags = VK_SHADER_STAGE_ALL,
                             },
                             {
                                 .binding = 1,
                                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 .descriptorCount = 1,
                                 .stageFlags = VK_SHADER_STAGE_ALL,
                             }},
    };
    return vklCreateGraphicsPipeline(graphics_pipeline_config);
}

std::vector<std::vector<VkPipeline>> createVkPipelineMatrix(PipelineParams &params, std::vector<VkPolygonMode> &polygonModes, std::vector<VkCullModeFlags> &cullingModes)
{
    auto m = std::vector<std::vector<VkPipeline>>(polygonModes.size());
    for (int i = 0; i < polygonModes.size(); i++)
    {
        m[i] = std::vector<VkPipeline>(cullingModes.size());
        for (int j = 0; j < cullingModes.size(); j++)
        {
            params.polygonMode = polygonModes[i];
            params.cullingMode = cullingModes[j];
            m[i][j] = createVkPipeline(params);
        }
    }
    return m;
}

void destroyVkPipelineMatrix(std::vector<std::vector<VkPipeline>> matrix)
{
    for (auto &&row : matrix)
    {
        for (auto &&entry : row)
        {
            vklDestroyGraphicsPipeline(entry);
        }
    }
}

class OrbitControls
{
private:
    float azimuth = 0.0f;
    float elevation = 0.0f;
    float distance = 5.0f;
    glm::vec3 center = glm::vec3(0.0f);
    std::shared_ptr<Camera> camera = nullptr;

public:
    OrbitControls(std::shared_ptr<Camera> camera)
    {
        azimuth = -camera->angles.y;
        elevation = camera->angles.x;
        this->camera = camera;
    }

    void update()
    {
        auto input = Input::instance();
        distance -= input->scrollDelta().y / 5.0f;
        distance = glm::clamp(distance, 0.1f, 100.0f);

        if (input->isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            auto delta = input->mouseDelta();
            azimuth -= delta.x / 200.0f;
            azimuth = glm::mod(glm::mod(azimuth, glm::two_pi<float>()) + glm::two_pi<float>(), glm::two_pi<float>());
            elevation += delta.y / 200.0f;
            elevation = glm::clamp(elevation, -glm::half_pi<float>(), glm::half_pi<float>());
        }

        camera->position = center;
        camera->position += glm::vec3(
            glm::sin(azimuth) * glm::cos(elevation),
            glm::sin(elevation),
            glm::cos(azimuth) * glm::cos(elevation));
        camera->position *= distance;
        camera->angles.x = -1.0f * elevation;
        camera->angles.y = 1.0f * azimuth;
        camera->updateView();
    }
};

class PipelineMatrixManager : public ITrash
{
private:
    std::vector<VkPolygonMode> polygon_modes = std::vector<VkPolygonMode>({
        VK_POLYGON_MODE_FILL,
        VK_POLYGON_MODE_LINE,
    });
    std::vector<VkCullModeFlags> culling_modes = std::vector<VkCullModeFlags>({
        VK_CULL_MODE_NONE,
        VK_CULL_MODE_BACK_BIT,
        VK_CULL_MODE_FRONT_BIT,
    });
    int polygon_mode = 0;
    int culling_mode = 0;
    std::vector<std::vector<VkPipeline>> matrix;

public:
    PipelineMatrixManager(std::string vshName, std::string fshName)
    {
        std::string vertShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/" + vshName);
        std::string fragShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/" + fshName);
        PipelineParams pipelineParams = {
            .vertexShaderPath = vertShaderPath,
            .fragmentShaderPath = fragShaderPath,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullingMode = VK_CULL_MODE_NONE,
        };

        matrix = createVkPipelineMatrix(pipelineParams, polygon_modes, culling_modes);
    }

    virtual void destroy()
    {
        destroyVkPipelineMatrix(matrix);
    }

    void set_polygon_mode(int mode)
    {
        polygon_mode = (mode + polygon_modes.size()) % polygon_modes.size();
    }

    void set_culling_mode(int mode)
    {
        culling_mode = (mode + culling_modes.size()) % culling_modes.size();
    }

    void update()
    {
        auto input = Input::instance();

        if (input->isKeyTap(GLFW_KEY_F1))
            set_polygon_mode(polygon_mode + 1);

        if (input->isKeyTap(GLFW_KEY_F2))
            set_culling_mode(culling_mode + 1);
    }

    VkPipeline selected()
    {
        return matrix[polygon_mode][culling_mode];
    }
};

std::unique_ptr<PipelineMatrixManager> createPipelineManager(std::string init_renderer_filepath)
{
    auto manager = std::make_unique<PipelineMatrixManager>("task2.vert", "task2.frag");

    INIReader renderer_reader(init_renderer_filepath);
    bool as_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
    if (as_wireframe)
        manager->set_polygon_mode(1);
    bool with_backface_culling = renderer_reader.GetBoolean("renderer", "backface_culling", false);
    if (with_backface_culling)
        manager->set_culling_mode(1);

    return manager;
}

class Mesh : public ITrash
{
private:
    VkBuffer vertices = VK_NULL_HANDLE;
    VkBuffer indices = VK_NULL_HANDLE;
    uint32_t index_count;

public:
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
    {
        this->vertices = vklCreateHostCoherentBufferWithBackingMemory(vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        vklCopyDataIntoHostCoherentBuffer(this->vertices, &vertices.front(), vertices.size() * sizeof(Vertex));
        this->indices = vklCreateHostCoherentBufferWithBackingMemory(indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        vklCopyDataIntoHostCoherentBuffer(this->indices, &indices.front(), indices.size() * sizeof(uint32_t));
        this->index_count = indices.size();
    }

    virtual void destroy()
    {
        vklDestroyHostCoherentBufferAndItsBackingMemory(vertices);
        vklDestroyHostCoherentBufferAndItsBackingMemory(indices);
    }

    void bind(VkCommandBuffer cmd_buffer)
    {
        VkDeviceSize vertex_offset = 0;
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertices, &vertex_offset);
        vkCmdBindIndexBuffer(cmd_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
    }

    void draw(VkCommandBuffer cmd_buffer)
    {
        vkCmdDrawIndexed(cmd_buffer, index_count, 1, 0, 0, 0);
    }
};

class MeshInstance
{
private:
    MeshInstanceUniformBlock uniform_block = {
        .color = {1.0, 1.0, 1.0, 1.0},
        .modelMatrix = glm::mat4(1.0),
    };
    VkBuffer uniform_buffer = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    UniformBufferSlot uniform_slot = {};

public:
    std::shared_ptr<Mesh> mesh = nullptr;

    MeshInstance(std::shared_ptr<Mesh> mesh)
    {
        this->mesh = mesh;
    }

    void init_uniforms(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_layout, uint32_t binding, VkBuffer uniform_buffer, UniformBufferSlot slot)
    {
        this->descriptor_set = createVkDescriptorSet(device, descriptor_pool, descriptor_layout);
        writeDescriptorSetBuffer(device, descriptor_set, binding, uniform_buffer, sizeof(uniform_block), slot);
        this->uniform_buffer = uniform_buffer;
        this->uniform_slot = slot;
        set_uniforms(uniform_block);
    }

    void set_uniforms(MeshInstanceUniformBlock data)
    {
        uniform_block = data;
        if (uniform_buffer != VK_NULL_HANDLE)
            vklCopyDataIntoHostCoherentBuffer(uniform_buffer, uniform_slot.offset, &uniform_block, uniform_slot.size);
    }

    void bind_uniforms(VkCommandBuffer cmd_buffer, VkPipelineLayout pipeline_layout)
    {
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
    }

    VkDescriptorSet get_descriptor_set()
    {
        return descriptor_set;
    }
};

std::unique_ptr<Mesh> createCylinderMesh(float radius, float height, int segments, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.push_back({{0, -height / 2, 0}, color});
    vertices.push_back({{0, height / 2, 0}, color});

    const int bot_start = 2;
    const int bot_end = 2 + segments - 1;
    const int top_start = 2 + segments;
    const int top_end = 2 + segments * 2 - 1;

    for (int half = 0; half < 2; half++)
    {
        bool top = half == 1;
        for (int s = 0; s < segments; s++)
        {
            float phi = glm::two_pi<float>() * s / segments;
            glm::vec3 v = {
                glm::cos(phi) * radius,
                top ? height / 2 : -height / 2,
                glm::sin(phi) * radius,
            };

            vertices.push_back({v, color});

            if (top && s > 0)
            {
                indices.push_back(0);
                indices.push_back(bot_start + s - 1);
                indices.push_back(bot_start + s);

                indices.push_back(top_start + s - 1);
                indices.push_back(bot_start + s);
                indices.push_back(bot_start + s - 1);

                indices.push_back(1);
                indices.push_back(top_start + s);
                indices.push_back(top_start + s - 1);

                indices.push_back(bot_start + s);
                indices.push_back(top_start + s - 1);
                indices.push_back(top_start + s);
            }
        }
    }

    indices.push_back(0);
    indices.push_back(bot_end);
    indices.push_back(bot_start);

    indices.push_back(top_end);
    indices.push_back(bot_start);
    indices.push_back(bot_end);

    indices.push_back(1);
    indices.push_back(top_start);
    indices.push_back(top_end);

    indices.push_back(bot_start);
    indices.push_back(top_end);
    indices.push_back(top_start);

    return make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createSphereMesh(float radius, int rings, int segments, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.push_back({{0, -radius, 0}, color});
    vertices.push_back({{0, radius, 0}, color});

    int prev_ring = 0;
    int curr_ring = 2;

    for (int r = 1; r < rings; r++)
    {
        bool cap = r == 1 || r == rings - 1;
        bool top_cap = r == rings - 1;
        float theta = glm::pi<float>() * r / rings;
        for (int s = 0; s < segments; s++)
        {
            float phi = glm::two_pi<float>() * s / segments;

            glm::vec3 v = {
                radius * glm::sin(theta) * glm::cos(phi),
                radius * -glm::cos(theta),
                radius * glm::sin(theta) * glm::sin(phi),
            };

            vertices.push_back({v, color});

            if (s == 0)
                continue;

            if (cap)
            {
                indices.push_back(curr_ring + s - 1);
                if (top_cap)
                    indices.push_back(1);
                indices.push_back(curr_ring + s);
                if (!top_cap)
                    indices.push_back(0);
            }
            if (r > 1)
            {
                indices.push_back(curr_ring + s - 1);
                indices.push_back(curr_ring + s);
                indices.push_back(prev_ring + s - 1);

                indices.push_back(prev_ring + s);
                indices.push_back(prev_ring + s - 1);
                indices.push_back(curr_ring + s);
            }
        }
        if (cap)
        {
            indices.push_back(curr_ring + segments - 1);
            if (top_cap)
                indices.push_back(1);
            indices.push_back(curr_ring);
            if (!top_cap)
                indices.push_back(0);
        }
        if (r > 1)
        {
            indices.push_back(curr_ring + segments - 1);
            indices.push_back(curr_ring);
            indices.push_back(prev_ring + segments - 1);

            indices.push_back(prev_ring);
            indices.push_back(prev_ring + segments - 1);
            indices.push_back(curr_ring);
        }
        prev_ring = curr_ring;
        curr_ring += segments;
    }

    return make_unique<Mesh>(vertices, indices);
}

class BezierCurve
{
private:
    std::vector<float> coefficients;
    std::vector<float> derivative_coefficients;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> derivative_points;

    void generate_coefficients()
    {
        uint32_t n = points.size() - 1;
        for (uint32_t i = 0; i <= n; i++)
        {
            coefficients.push_back(binom(n, i));
            if (i <= n - 1)
            {
                derivative_coefficients.push_back(binom(n - 1, i));
                derivative_points.push_back(points[i + 1] - points[i]);
            }
        }
    }

    uint64_t binom(uint32_t n, uint32_t k)
    {
        return fact(n) / (fact(k) * fact(n - k));
    }

    uint64_t fact(uint32_t n)
    {
        uint64_t factorial = 1;
        for (int i = 1; i <= n; ++i)
        {
            factorial *= i;
        }
        return factorial;
    }

    float power_at(float t, int n, int i)
    {
        if (i == 0)
            return std::pow(1.0 - t, n - i);
        if (i == n)
            return std::pow(t, i);
        return std::pow(1.0 - t, n - i) * std::pow(t, i);
    }

public:
    BezierCurve(std::vector<glm::vec3> points)
    {
        this->points = points;
        this->generate_coefficients();
    }

    glm::vec3 value_at(float t)
    {
        glm::vec3 sum = {};
        uint32_t n = points.size() - 1;
        for (size_t i = 0; i <= n; i++)
        {
            float w = coefficients[i] * power_at(t, n, i);
            sum += w * points[i];
        }
        return sum;
    }

    glm::vec3 tanget_at(float t)
    {
        glm::vec3 sum = {};
        uint32_t n = points.size() - 1;
        for (size_t i = 0; i <= n - 1; i++)
        {
            float w = derivative_coefficients[i] * power_at(t, n - 1, i);
            sum += w * derivative_points[i];
        }
        return float(n) * sum;
    }
};

std::unique_ptr<Mesh> createBezierMesh(std::unique_ptr<BezierCurve> curve, glm::vec3 up, float radius, int resolution, int segments, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.push_back({curve->value_at(1.0), color});
    vertices.push_back({curve->value_at(0.0), color});

    int prev_ring = 0;
    int curr_ring = 2;
    for (int r = 0; r <= resolution; r++)
    {
        bool cap = r == 0 || r == resolution;
        bool top_cap = r == resolution;
        float f = 1.0 - (float(r) / resolution);
        glm::vec3 p = curve->value_at(f);
        glm::vec3 t = curve->tanget_at(f);
        glm::vec3 bn = glm::normalize(glm::cross(t, up)) * radius;
        for (int s = 0; s < segments; s++)
        {
            float phi = glm::two_pi<float>() * s / segments;
            glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), phi, t)) * bn;
            glm::vec3 v = p + n;
            vertices.push_back({v, color});

            if (s == 0)
                continue;

            if (cap)
            {
                indices.push_back(curr_ring + s - 1);
                if (top_cap)
                    indices.push_back(1);
                indices.push_back(curr_ring + s);
                if (!top_cap)
                    indices.push_back(0);
            }
            if (r > 0)
            {
                indices.push_back(curr_ring + s - 1);
                indices.push_back(curr_ring + s);
                indices.push_back(prev_ring + s - 1);

                indices.push_back(prev_ring + s);
                indices.push_back(prev_ring + s - 1);
                indices.push_back(curr_ring + s);
            }
        }
        if (cap)
        {
            indices.push_back(curr_ring + segments - 1);
            if (top_cap)
                indices.push_back(1);
            indices.push_back(curr_ring);
            if (!top_cap)
                indices.push_back(0);
        }
        if (r > 0)
        {
            indices.push_back(curr_ring + segments - 1);
            indices.push_back(curr_ring);
            indices.push_back(prev_ring + segments - 1);

            indices.push_back(prev_ring);
            indices.push_back(prev_ring + segments - 1);
            indices.push_back(curr_ring);
        }
        prev_ring = curr_ring;
        curr_ring += segments;
    }

    return make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> createTorusMesh(float majorRadius, float minorRadius, int rings, int segments, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    int prev_ring = 0;
    int curr_ring = 0;
    int count = rings * segments;
    for (int r = 0; r <= rings; r++)
    {
        float theta = glm::two_pi<float>() * r / rings;
        glm::mat4 mat = glm::translate(
            glm::rotate(glm::mat4(1.0), theta, {0, 0, 1}),
            {0, majorRadius, 0});
        for (int s = 0; s < segments; s++)
        {
            if (r < rings)
            {
                float phi = glm::two_pi<float>() * s / segments;
                glm::vec4 n = {
                    0.0,
                    glm::cos(phi) * minorRadius,
                    glm::sin(phi) * minorRadius,
                    1.0,
                };
                glm::vec3 v = mat * n;
                vertices.push_back({v, color});
            }

            if (s > 0)
            {
                indices.push_back((curr_ring + s - 1 + count) % count);
                indices.push_back((curr_ring + s + count) % count);
                indices.push_back((prev_ring + s - 1 + count) % count);

                indices.push_back((prev_ring + s + count) % count);
                indices.push_back((prev_ring + s - 1 + count) % count);
                indices.push_back((curr_ring + s + count) % count);
            }
        }
        if (r > 0)
        {
            indices.push_back((curr_ring + segments - 1 + count) % count);
            indices.push_back((curr_ring + count) % count);
            indices.push_back((prev_ring + segments - 1 + count) % count);

            indices.push_back((prev_ring + count) % count);
            indices.push_back((prev_ring + segments - 1 + count) % count);
            indices.push_back((curr_ring + count) % count);
        }
        prev_ring = curr_ring;
        curr_ring += segments;
    }

    return make_unique<Mesh>(vertices, indices);
}

std::vector<std::unique_ptr<MeshInstance>> createScene()
{
    std::shared_ptr<Mesh> cornell_mesh = std::make_shared<Mesh>(createCornellVertices(3, 3, 3), cornell_indices);
    std::shared_ptr<Mesh> cube_mesh = std::make_shared<Mesh>(createCubeVertices(0.34, 0.34, 0.34, {1.0, 1.0, 1.0}), cube_indices);
    std::shared_ptr<Mesh> cylinder_mesh(createCylinderMesh(0.2, 1.5, 18, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> sphere_mesh(createSphereMesh(0.24, 8, 18, {1.0, 1.0, 1.0}));
    std::unique_ptr<BezierCurve> bezeier_curve(new BezierCurve({{-0.3f, 0.6f, 0.0f},
                                                                {0.0f, 1.6f, 0.0f},
                                                                {1.4f, 0.3f, 0.0f},
                                                                {0.0f, 0.3f, 0.0f},
                                                                {0.0f, -0.5f, 0.0f}}));
    std::shared_ptr<Mesh> bezier_mesh(createBezierMesh(std::move(bezeier_curve), {0, 0, -1}, 0.2, 42, 18, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> torus_mesh(createTorusMesh(1.1, 0.1, 32, 8, {1.0, 1.0, 1.0}));

    std::vector<std::unique_ptr<MeshInstance>> instances;
    MeshInstance *cornell_instance = new MeshInstance(cornell_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cornell_instance));

    MeshInstance *cube_instance = new MeshInstance(cube_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cube_instance));
    cube_instance->set_uniforms({
        .color = {0.7, 0.1, 0.2, 1.0},
        .modelMatrix =
            glm::rotate(
                glm::translate(glm::mat4(1.0), {-0.5, -0.8, 0.0}),
                glm::radians(45.0f), {0, 1, 0}),
    });

    MeshInstance *cylinder_instance = new MeshInstance(cylinder_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cylinder_instance));
    cylinder_instance->set_uniforms({
        .color = {0.2, 0.8, 0.4, 1.0},
        .modelMatrix = glm::translate(glm::mat4(1.0), {-0.5, 0.3, 0.0}),
    });

    MeshInstance *bezier_instance = new MeshInstance(bezier_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(bezier_instance));
    bezier_instance->set_uniforms({
        .color = {0.2, 0.8, 0.4, 1.0},
        .modelMatrix = glm::translate(glm::mat4(1.0), {0.5, 0, 0}),
    });

    MeshInstance *sphere_instance = new MeshInstance(sphere_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(sphere_instance));
    sphere_instance->set_uniforms({
        .color = {0.4, 0.3, 0.7, 1.0},
        .modelMatrix = glm::translate(glm::mat4(1.0), {0.5, -0.8, 0}),
    });

    MeshInstance *torus_instance = new MeshInstance(torus_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(torus_instance));
    torus_instance->set_uniforms({
        .color = {1.0, 0.3, 0.0, 1.0},
        .modelMatrix = glm::rotate(
            glm::scale(glm::mat4(1.0), {1.0, 1.5, 1.0}),
            glm::radians(45.0f), {1, 0, 0}),
    });

    return instances;
}

#pragma endregion

/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char **argv)
{
    VKL_LOG(":::::: WELCOME TO GCG 2023 ::::::");

#pragma region vulkan_setup
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
    std::vector<VkDetailedImage> swapchain_color_attachments;
    VkDetailedImage swapchain_depth_attachment = {};
    VkSurfaceFormatKHR vk_surface_image_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    VkSwapchainKHR vk_swapchain = createVkSwapchain(vk_physical_device, vk_device, vk_surface, vk_surface_image_format, window, graphics_queue_family, swapchain_color_attachments, &swapchain_depth_attachment);
#pragma endregion

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
    VklSwapchainConfig swapchain_config = createVklSwapchainConfig(vk_swapchain, swapchain_color_attachments, swapchain_depth_attachment);

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config))
    {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }

    std::string init_camera_filepath = "assets/settings/camera_front.ini";
    if (cmdline_args.init_camera)
        init_camera_filepath = cmdline_args.init_camera_filepath;
    std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
    if (cmdline_args.init_renderer)
        init_renderer_filepath = cmdline_args.init_renderer_filepath;

    std::shared_ptr<Camera> camera(createCamera(init_camera_filepath, window));
    trash.push_back(camera);
    std::shared_ptr<Input> input = Input::init(window);
    std::shared_ptr<OrbitControls> controls(new OrbitControls(camera));
    std::shared_ptr<PipelineMatrixManager> pipelines = createPipelineManager(init_renderer_filepath);
    trash.push_back(pipelines);

    // All instances share a uniform buffer
    std::shared_ptr<SharedUniformBuffer> uniform_buffer(new SharedUniformBuffer(vk_physical_device, sizeof(MeshInstanceUniformBlock), 20));
    trash.push_back(uniform_buffer);

    VkDescriptorPool vk_descriptor_pool = createVkDescriptorPool(vk_device, 20, 20 * 2);
    VkDescriptorSetLayout vk_descriptor_set_layout = createVkDescriptorSetLayout(
        vk_device,
        {{
             .binding = 0,
             .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         },
         {
             .binding = 1,
             .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         }});

    auto mesh_instances = createScene();
    for (size_t i = 0; i < mesh_instances.size(); i++)
    {
        mesh_instances[i]->init_uniforms(vk_device, vk_descriptor_pool, vk_descriptor_set_layout, 1, uniform_buffer->buffer, uniform_buffer->slot(i));
        trash.push_back(mesh_instances[i]->mesh);

        // vklCreateGraphicsPipeline does not allow binding multiple descriptor sets simultaneously
        // thus it's required to hook the scene-static uniforms into every descriptor set
        // See: https://github.com/cg-tuwien/VulkanLaunchpad/issues/30
        camera->init_uniforms(vk_device, mesh_instances[i]->get_descriptor_set(), 0);
    }

    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    while (!glfwWindowShouldClose(window))
    {
        // NOTE: input update need to be called before glfwPollEvents
        input->update();
        glfwPollEvents();

        if (input->isKeyTap(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        pipelines->update();
        controls->update();

        vklWaitForNextSwapchainImage();

        vklStartRecordingCommands();
        VkCommandBuffer vk_cmd_buffer = vklGetCurrentCommandBuffer();
        VkPipeline vk_selected_pipeline = pipelines->selected();
        vklCmdBindPipeline(vk_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_selected_pipeline);
        VkPipelineLayout vk_pipeline_layout = vklGetLayoutForPipeline(vk_selected_pipeline);

        for (auto &&i : mesh_instances)
        {
            i->bind_uniforms(vk_cmd_buffer, vk_pipeline_layout);
            i->mesh->bind(vk_cmd_buffer);
            i->mesh->draw(vk_cmd_buffer);
        }

        vklEndRecordingCommands();
        vklPresentCurrentSwapchainImage();

        if (cmdline_args.run_headless)
        {
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename)
                screenshot_filename = cmdline_args.filename;

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            gcgSaveScreenshot(screenshot_filename, swapchain_color_attachments[idx].image, width,
                              height, vk_surface_image_format.format, vk_device, vk_physical_device, vk_queue,
                              graphics_queue_family);
            break;
        }
    }

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);
    vkDestroyDescriptorSetLayout(vk_device, vk_descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, nullptr);
    vklDestroyDeviceLocalImageAndItsBackingMemory(swapchain_depth_attachment.image);
    for (auto &&i : trash)
    {
        i->destroy();
    }
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