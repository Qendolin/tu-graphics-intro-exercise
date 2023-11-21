/*
 * Copyright 2023 TU Wien, Institute of Visual Computing & Human-Centered Technology.
 * This file is part of the GCG Lab Framework and must not be redistributed.
 *
 * Original version created by Lukas Gersthofer and Bernhard Steiner.
 * Vulkan edition created by Johannes Unterguggenberger (junt@cg.tuwien.ac.at).
 */
#include "PathUtils.h"
#include "Utils.h"
#include "Mesh.h"
#include "MyUtils.h"
#include "Camera.h"
#include "Setup.h"
#include "Descriptors.h"
#include "Pipelines.h"
#include "Input.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>

#undef min
#undef max

std::vector<std::shared_ptr<ITrash>> trash;

#pragma region hide_this_stuff

std::vector<std::unique_ptr<MeshInstance>> createScene()
{
    std::shared_ptr<Mesh> cornell_mesh(create_cornell_mesh(3, 3, 3));
    std::shared_ptr<Mesh> cube_mesh(create_cube_mesh(0.34, 0.34, 0.34, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> cylinder_mesh(create_cylinder_mesh(0.2, 1.5, 18, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> sphere_mesh(create_sphere_mesh(0.24, 8, 18, {1.0, 1.0, 1.0}));
    std::unique_ptr<BezierCurve> bezeier_curve(new BezierCurve({{-0.3f, 0.6f, 0.0f},
                                                                {0.0f, 1.6f, 0.0f},
                                                                {1.4f, 0.3f, 0.0f},
                                                                {0.0f, 0.3f, 0.0f},
                                                                {0.0f, -0.5f, 0.0f}}));
    std::shared_ptr<Mesh> bezier_mesh(create_bezier_mesh(std::move(bezeier_curve), {0, 0, -1}, 0.2, 42, 18, {1.0, 1.0, 1.0}));

    std::vector<std::unique_ptr<MeshInstance>> instances;
    MeshInstance *cornell_instance = new MeshInstance(cornell_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cornell_instance));

    MeshInstance *cube_instance = new MeshInstance(cube_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cube_instance));
    cube_instance->set_uniforms({
        .color = {0.7, 0.1, 0.2, 1.0},
        .model_matrix =
            glm::rotate(
                glm::translate(glm::mat4(1.0), {-0.5, -0.8, 0.0}),
                glm::radians(45.0f), {0, 1, 0}),
    });

    MeshInstance *cylinder_instance = new MeshInstance(cylinder_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(cylinder_instance));
    cylinder_instance->set_uniforms({
        .color = {0.2, 0.8, 0.4, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {-0.5, 0.3, 0.0}),
    });

    MeshInstance *bezier_instance = new MeshInstance(bezier_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(bezier_instance));
    bezier_instance->set_uniforms({
        .color = {0.2, 0.8, 0.4, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {0.5, 0, 0}),
    });

    MeshInstance *sphere_instance = new MeshInstance(sphere_mesh);
    instances.push_back(std::unique_ptr<MeshInstance>(sphere_instance));
    sphere_instance->set_uniforms({
        .color = {0.4, 0.3, 0.7, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {0.5, -0.8, 0}),
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
