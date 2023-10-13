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
    VkDescriptorPoolCreateInfo descriptorÜoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize,
    };
    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    VkResult error = vkCreateDescriptorPool(vkDevice, &descriptorÜoolCreateInfo, nullptr, &vkDescriptorPool);
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

void writeDescriptorSetBuffer(VkDevice vkDevice, VkDescriptorSet dst, uint32_t binding, VkBuffer buffer, size_t size, uint32_t start = 0, uint32_t count = -1)
{
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = buffer,
        .offset = start * size,
        .range = count == -1 ? VK_WHOLE_SIZE : count * size,
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

class Camera
{
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
    glm::mat4 viewProjectionMatrix;

    Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles)
    {
        this->fovRad = fovRad;
        this->viewportSize = viewportSize;
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;
        this->position = position;
        this->angles = angles;
        updateProjection();
        updateView();
    }

    void updateProjection()
    {
        float aspect = viewportSize.x / viewportSize.y;
        projectionMatrix = gcgCreatePerspectiveProjectionMatrix(fovRad, aspect, nearPlane, farPlane);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
    }

    void updateView()
    {
        viewMatrix = glm::mat4(1.0f);
        viewMatrix = glm::translate(viewMatrix, position);
        viewMatrix = glm::rotate(viewMatrix, angles.z, {0, 0, 1});
        viewMatrix = glm::rotate(viewMatrix, angles.y, {0, 1, 0});
        viewMatrix = glm::rotate(viewMatrix, angles.x, {1, 0, 0});
        viewMatrix = glm::inverse(viewMatrix);
        viewProjectionMatrix = projectionMatrix * viewMatrix;
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
    static Input *_instance;

    float _prevTime;
    float _time;
    float _timeDelta;
    glm::vec2 _mousePrevPos;
    glm::vec2 _mousePos;
    glm::vec2 _mouseDelta;
    glm::vec2 _scrollDelta;
    glm::vec2 _scrollNextDelta;
    bool _mousePrevButtons[GLFW_MOUSE_BUTTON_LAST];
    bool _mouseButtons[GLFW_MOUSE_BUTTON_LAST];

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

    void update()
    {
        _time = glfwGetTime();
        _timeDelta = _time - _prevTime;
        _mouseDelta = _mousePos - _mousePrevPos;
        _mousePrevPos = _mousePos;
        _scrollDelta = _scrollNextDelta;
        _scrollNextDelta = glm::vec2(0.0f);
        std::copy(std::begin(_mouseButtons), std::end(_mouseButtons), std::begin(_mousePrevButtons));
    }

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
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

    static Input *instance();
    static Input *init(GLFWwindow *window);
};

Input *Input::_instance = nullptr;

Input *Input::instance()
{

    return Input::_instance;
}

Input *Input::init(GLFWwindow *window)
{
    Input::_instance = new Input();
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

#pragma endregion

struct Vertex
{
    glm::vec3 position;
};

struct ModelUniformBlock
{
    glm::vec4 color;
    glm::mat4 modelMatrix;
};

struct CameraUniformBlock
{
    glm::mat4 viewProjectionMatrix;
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
    std::vector<VkDetailedImage> swapchain_color_attachments;
    VkDetailedImage swapchain_depth_attachment = {};
    VkSurfaceFormatKHR vk_surface_image_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    VkSwapchainKHR vk_swapchain = createVkSwapchain(vk_physical_device, vk_device, vk_surface, vk_surface_image_format, window, graphics_queue_family, swapchain_color_attachments, &swapchain_depth_attachment);

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

    std::string init_camera_filepath = "assets/settings/camera_front_right.ini";
    if (cmdline_args.init_camera)
    {
        init_camera_filepath = cmdline_args.init_camera_filepath;
    }
    auto camera = createCamera(init_camera_filepath, window);
    Input *input = Input::init(window);
    // azimuth & elevation
    glm::vec2 orbitDirection = glm::vec2(-camera->angles.y, camera->angles.x);
    float orbitDistance = 5.0f;
    glm::vec3 orgbitCenter = glm::vec3(0.0f);

    std::string vertShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/task2.vert");
    std::string fragShaderPath = gcgLoadShaderFilePath("assets/shaders_vk/task2.frag");
    VklGraphicsPipelineConfig graphics_pipeline_config = {
        .vertexShaderPath = vertShaderPath.c_str(),
        .fragmentShaderPath = fragShaderPath.c_str(),
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
        .polygonDrawMode = VK_POLYGON_MODE_LINE,
        .triangleCullingMode = VK_CULL_MODE_BACK_BIT,
        .descriptorLayout = {
            {
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
    VkPipeline vk_pipeline = vklCreateGraphicsPipeline(graphics_pipeline_config);
    glm::mat4 model_matrix_1 = glm::mat4(1.0);
    model_matrix_1 = glm::translate(model_matrix_1, {-1.5, 1, 0});
    model_matrix_1 = glm::rotate(model_matrix_1, glm::radians(180.0f), {0, 1, 0});
    glm::mat4 model_matrix_2 = glm::mat4(1.0);
    model_matrix_2 = glm::translate(model_matrix_2, {1.5, -1, 0});
    model_matrix_2 = glm::scale(model_matrix_2, {1, 2, 1});
    ModelUniformBlock uniform_data_1 = {
        .color = {0.2, 0.6, 0.4, 1.0},
        .modelMatrix = model_matrix_1,
    };
    ModelUniformBlock uniform_data_2 = {
        .color = {0.7, 0.1, 0.2, 1.0},
        .modelMatrix = model_matrix_2,
    };
    VkBuffer uniform_buffer_1 = vklCreateHostCoherentBufferWithBackingMemory(sizeof(ModelUniformBlock), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    VkBuffer uniform_buffer_2 = vklCreateHostCoherentBufferWithBackingMemory(sizeof(ModelUniformBlock), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    VkBuffer uniform_buffer_3 = vklCreateHostCoherentBufferWithBackingMemory(sizeof(CameraUniformBlock), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(uniform_buffer_1, &uniform_data_1, sizeof(ModelUniformBlock));
    vklCopyDataIntoHostCoherentBuffer(uniform_buffer_2, &uniform_data_2, sizeof(ModelUniformBlock));

    VkDescriptorPool vk_descriptor_pool = createVkDescriptorPool(vk_device, 8, 16);
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

    VkDescriptorSet vk_descriptor_set_1 = createVkDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    VkDescriptorSet vk_descriptor_set_2 = createVkDescriptorSet(vk_device, vk_descriptor_pool, vk_descriptor_set_layout);
    writeDescriptorSetBuffer(vk_device, vk_descriptor_set_1, 0, uniform_buffer_1, sizeof(ModelUniformBlock));
    writeDescriptorSetBuffer(vk_device, vk_descriptor_set_1, 1, uniform_buffer_3, sizeof(CameraUniformBlock));
    writeDescriptorSetBuffer(vk_device, vk_descriptor_set_2, 0, uniform_buffer_2, sizeof(ModelUniformBlock));
    writeDescriptorSetBuffer(vk_device, vk_descriptor_set_2, 1, uniform_buffer_3, sizeof(CameraUniformBlock));

    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        input->update();

        orbitDistance -= input->scrollDelta().y / 5.0f;
        orbitDistance = glm::clamp(orbitDistance, 0.1f, 100.0f);

        if (input->isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
        {
            auto delta = input->mouseDelta();
            orbitDirection.x -= delta.x / 200.0f;
            orbitDirection.x = glm::mod(glm::mod(orbitDirection.x, glm::two_pi<float>()) + glm::two_pi<float>(), glm::two_pi<float>());
            orbitDirection.y += delta.y / 200.0f;
            orbitDirection.y = glm::clamp(orbitDirection.y, -glm::half_pi<float>(), glm::half_pi<float>());
        }

        camera->position = orgbitCenter;
        camera->position += glm::vec3(
            glm::sin(orbitDirection.x) * glm::cos(orbitDirection.y),
            glm::sin(orbitDirection.y),
            glm::cos(orbitDirection.x) * glm::cos(orbitDirection.y));
        camera->position *= orbitDistance;
        camera->angles.x = -1.0f * orbitDirection.y;
        camera->angles.y = 1.0f * orbitDirection.x;
        camera->updateView();

        CameraUniformBlock camera_uniform_data = {
            .viewProjectionMatrix = camera->viewProjectionMatrix,
        };
        vklCopyDataIntoHostCoherentBuffer(uniform_buffer_3, &camera_uniform_data, sizeof(CameraUniformBlock));

        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();
        gcgDrawTeapot(vk_pipeline, vk_descriptor_set_1);
        gcgDrawTeapot(vk_pipeline, vk_descriptor_set_2);
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
    vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer_1);
    vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer_2);
    vklDestroyHostCoherentBufferAndItsBackingMemory(uniform_buffer_3);
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