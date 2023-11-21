#pragma once

#include <vulkan/vulkan.h>
#include <VulkanLaunchpad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include "MyUtils.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <memory>

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
void errorCallbackFromGlfw(int error, const char *description);

GLFWwindow *createGLFWWindow();
VkInstance createVkInstance();
VkSurfaceKHR createVkSurface(VkInstance vkInstance, GLFWwindow *window);
VkPhysicalDevice createVkPhysicalDevice(VkInstance vkInstance, VkSurfaceKHR vkSurface);
VkDevice createVkDevice(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily);
VkSwapchainKHR createVkSwapchain(VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkSurfaceKHR vkSurface, VkSurfaceFormatKHR vkSurfaceImageFormat, GLFWwindow *window, uint32_t queueFamily, std::vector<VkDetailedImage> &colorAttachments, VkDetailedImage *depthAttachment);
VklSwapchainConfig createVklSwapchainConfig(VkSwapchainKHR vkSwapchain, std::vector<VkDetailedImage> &colorAttachments, VkDetailedImage &depthAttachment);
