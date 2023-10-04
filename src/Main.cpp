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

/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char **argv)
{
    VKL_LOG(":::::: WELCOME TO GCG 2023 ::::::");

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

    /* --------------------------------------------- */
    // Subtask 1.1: Load Settings From File
    /* --------------------------------------------- */

    INIReader window_reader("assets/settings/window.ini");

    int window_width = window_reader.GetInteger("window", "width", 800);
    int window_height = window_reader.GetInteger("window", "height", 800);
    std::string window_title = window_reader.Get("window", "title", "GCG 2023");
    bool fullscreen = false;
    // Install a callback function, which gets invoked whenever a GLFW error occurred.
    glfwSetErrorCallback(errorCallbackFromGlfw);

    /* --------------------------------------------- */
    // Subtask 1.2: Create a Window with GLFW
    /* --------------------------------------------- */
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

    GLFWwindow *window = glfwCreateWindow(window_width, window_height, window_title.c_str(), monitor, nullptr);

    if (!window)
    {
        VKL_LOG("If your program reaches this point, that means two things:");
        VKL_LOG("1) Project setup was successful. Everything is working fine.");
        VKL_LOG("2) You haven't implemented Subtask 1.2, which is creating a window with GLFW.");
        VKL_EXIT_WITH_ERROR("No GLFW window created.");
    }
    VKL_LOG("Subtask 1.2 done.");

    VkResult error;
    VkInstance vk_instance = VK_NULL_HANDLE;              // To be set during Subtask 1.3
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;             // To be set during Subtask 1.4
    VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE; // To be set during Subtask 1.5
    VkDevice vk_device = VK_NULL_HANDLE;                  // To be set during Subtask 1.7
    VkQueue vk_queue = VK_NULL_HANDLE;                    // To be set during Subtask 1.7
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;         // To be set during Subtask 1.8

    /* --------------------------------------------- */
    // Subtask 1.3: Create a Vulkan Instance
    /* --------------------------------------------- */
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

    error = vkCreateInstance(&instance_create_info, nullptr, &vk_instance);
    VKL_CHECK_VULKAN_ERROR(error);

    if (!vk_instance)
    {
        VKL_EXIT_WITH_ERROR("No VkInstance created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.3 done.");

    /* --------------------------------------------- */
    // Subtask 1.4: Create a Vulkan Window Surface
    /* --------------------------------------------- */

    glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface);
    if (!vk_surface)
    {
        VKL_EXIT_WITH_ERROR("No VkSurfaceKHR created or handle not assigned.");
    }
    VKL_LOG("Subtask 1.4 done.");

    /* --------------------------------------------- */
    // Subtask 1.5: Pick a Physical Device
    /* --------------------------------------------- */
    // TODO: Use vkEnumeratePhysicalDevices get all the available physical device handles!
    //       Select one that is suitable using the helper function selectPhysicalDeviceIndex and assign it to vk_physical_device!
    uint32_t physical_device_count;
    error = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(error);

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    error = vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, &physical_devices.front());
    VKL_CHECK_VULKAN_ERROR(error);

    vk_physical_device = physical_devices[selectPhysicalDeviceIndex(physical_devices, vk_surface)];

    if (!vk_physical_device)
    {
        VKL_EXIT_WITH_ERROR("No VkPhysicalDevice selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.5 done.");

    /* --------------------------------------------- */
    // Subtask 1.6: Select a Queue Family
    /* --------------------------------------------- */
    // TODO: Find a suitable queue family and assign its index to the following variable:
    //       Hint: Use the function selectQueueFamilyIndex, but complete its implementation first!
    uint32_t selected_queue_family_index = selectQueueFamilyIndex(vk_physical_device, vk_surface);

    // Sanity check if we have selected a valid queue family index:
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);
    if (selected_queue_family_index >= queue_family_count)
    {
        VKL_EXIT_WITH_ERROR("Invalid queue family index selected.");
    }
    VKL_LOG("Subtask 1.6 done.");

    /* --------------------------------------------- */
    // Subtask 1.7: Create a Logical Device and Get Queue
    /* --------------------------------------------- */
    // TODOs: - Create an instance of VkDeviceCreateInfo and use it to create one queue!
    //        - Hook in the address of the VkDeviceQueueCreateInfo instance at the right place!
    //        - Use VkDeviceCreateInfo::enabledExtensionCount and VkDeviceCreateInfo::ppEnabledExtensionNames
    //          to enable the VK_KHR_SWAPCHAIN_EXTENSION_NAME device extension!
    //        - Ensure that the other settings (which are unused in our case) are zero-initialized!
    //        - Finally, use vkCreateDevice to create the device and assign its handle to vk_device!

    float queue_priority = 1.0f;
    const char *required_device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = selected_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = &required_device_extensions;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;

    error = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
    VKL_CHECK_VULKAN_ERROR(error);
    if (!vk_device)
    {
        VKL_EXIT_WITH_ERROR("No VkDevice created or handle not assigned.");
    }

    // TODO: After device creation, use vkGetDeviceQueue to get the one and only created queue!
    //       Assign its handle to vk_queue!

    vkGetDeviceQueue(vk_device, selected_queue_family_index, 0, &vk_queue);

    if (!vk_queue)
    {
        VKL_EXIT_WITH_ERROR("No VkQueue selected or handle not assigned.");
    }
    VKL_LOG("Subtask 1.7 done.");

    /* --------------------------------------------- */
    // Subtask 1.8: Create a Swapchain
    /* --------------------------------------------- */
    uint32_t queueFamilyIndexCount = 1u;
    std::vector<uint32_t> queueFamilyIndices({selected_queue_family_index});
    VkSurfaceCapabilitiesKHR surface_capabilities = getPhysicalDeviceSurfaceCapabilities(vk_physical_device, vk_surface);
    // Build the swapchain config struct:
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = vk_surface;
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
    swapchain_create_info.queueFamilyIndexCount = queueFamilyIndexCount;
    swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices.data();
    // TODO: Provide values for:
    //        - VkSwapchainCreateInfoKHR::queueFamilyIndexCount
    //        - VkSwapchainCreateInfoKHR::pQueueFamilyIndices
    //        - VkSwapchainCreateInfoKHR::imageFormat
    //        - VkSwapchainCreateInfoKHR::imageColorSpace
    //        - VkSwapchainCreateInfoKHR::imageExtent
    //        - VkSwapchainCreateInfoKHR::presentMode
    VkSurfaceFormatKHR surface_image_format = getSurfaceImageFormat(vk_physical_device, vk_surface);
    swapchain_create_info.imageFormat = surface_image_format.format;
    swapchain_create_info.imageColorSpace = surface_image_format.colorSpace;
    swapchain_create_info.imageExtent = {(uint32_t)window_width, (uint32_t)window_height};
    swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    // TODO: Create the swapchain using vkCreateSwapchainKHR and assign its handle to vk_swapchain!
    error = vkCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &vk_swapchain);
    VKL_CHECK_VULKAN_ERROR(error);

    if (!vk_swapchain)
    {
        VKL_EXIT_WITH_ERROR("No VkSwapchainKHR created or handle not assigned.");
    }

    // TODO: Use vkGetSwapchainImagesKHR to retrieve the created VkImage handles and store them in a collection (e.g., std::vector)!
    uint32_t swapchain_image_count = 0u;
    error = vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, nullptr);
    VKL_CHECK_VULKAN_ERROR(error);
    std::vector<VkImage> swapchain_images(swapchain_image_count);
    error = vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_image_count, &swapchain_images.front());
    VKL_CHECK_VULKAN_ERROR(error);

    if (swapchain_image_count != surface_capabilities.minImageCount)
    {
        VKL_LOG("Swapchain image count does NOT match! " << std::to_string(swapchain_image_count) << " != " << std::to_string(surface_capabilities.minImageCount));
    }

    VKL_LOG("Subtask 1.8 done.");

    /* --------------------------------------------- */
    // Subtask 1.9: Init GCG Framework
    /* --------------------------------------------- */

    std::vector<VklSwapchainFramebufferComposition> swapchain_image_compositions;
    for (size_t i = 0; i < swapchain_image_count; i++)
    {
        VklSwapchainFramebufferComposition composition = {
            .colorAttachmentImageDetails = {
                .imageHandle = swapchain_images[i],
                .imageFormat = surface_image_format.format,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .clearValue = {0.8, 1.0, 1.0, 1.0},
            },
            .depthAttachmentImageDetails = {},
        };
        swapchain_image_compositions.push_back(composition);
    }

    // Gather swapchain config as required by the framework:
    VklSwapchainConfig swapchain_config = {
        .swapchainHandle = vk_swapchain,
        .imageExtent = {(uint32_t)window_width, (uint32_t)window_height},
        .swapchainImages = swapchain_image_compositions,
    };

    // Init the framework:
    if (!gcgInitFramework(vk_instance, vk_surface, vk_physical_device, vk_device, vk_queue, swapchain_config))
    {
        VKL_EXIT_WITH_ERROR("Failed to init framework");
    }
    VKL_LOG("Subtask 1.9 done.");

    /* --------------------------------------------- */
    // Subtask 1.10: Set-up the Render Loop
    // Subtask 1.11: Register a Key Callback
    /* --------------------------------------------- */

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } });

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        vklWaitForNextSwapchainImage();
        vklStartRecordingCommands();
        gcgDrawTeapot();
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
            gcgSaveScreenshot(screenshot_filename, swapchain_images[idx], window_width,
                              window_height, surface_image_format.format, vk_device, vk_physical_device, vk_queue,
                              selected_queue_family_index);
            break;
        }
    }

    // Wait for all GPU work to finish before cleaning up:
    vkDeviceWaitIdle(vk_device);

    /* --------------------------------------------- */
    // Subtask 1.12: Cleanup
    /* --------------------------------------------- */
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
