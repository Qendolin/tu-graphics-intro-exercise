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
#include "Texture.h"
#include "vulkan_ext.h"

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

struct ShaderConstantsUniformBlock
{
    glm::ivec4 user_input;
};

struct DirectionalLightUniformBlock
{
    glm::vec4 direction;
    glm::vec4 color;
};

struct PointLightUniformBlock
{
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 attenuation;
};

std::vector<std::unique_ptr<MeshInstance>> createScene()
{
    std::shared_ptr<Mesh> cornell_mesh(create_cornell_mesh(3, 3, 3));
    std::shared_ptr<Mesh> cube_mesh(create_cube_mesh(0.34, 0.34, 0.34, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> cylinder_mesh(create_cylinder_mesh(0.2, 1.5, 18, {1.0, 1.0, 1.0}));
    std::shared_ptr<Mesh> sphere_mesh(create_sphere_mesh(0.24, 16, 32, {1.0, 1.0, 1.0}));
    std::unique_ptr<BezierCurve> bezeier_curve(new BezierCurve({{-0.3f, 0.6f, 0.0f},
                                                                {0.0f, 1.6f, 0.0f},
                                                                {1.4f, 0.3f, 0.0f},
                                                                {0.0f, 0.3f, 0.0f},
                                                                {0.0f, -0.5f, 0.0f}}));
    std::shared_ptr<Mesh> bezier_mesh(create_bezier_mesh(std::move(bezeier_curve), {0, 0, -1}, 0.2, 42, 18, {1.0, 1.0, 1.0}));

    std::vector<std::unique_ptr<MeshInstance>> instances;
    MeshInstance *cornell_instance = new MeshInstance(cornell_mesh, PipelineMatrixManager::Shader::Box);
    instances.push_back(std::unique_ptr<MeshInstance>(cornell_instance));
    cornell_instance->set_uniforms({
        .color = {1.0, 1.0, 1.0, 1.0},
        .model_matrix = glm::mat4(1.0),
        .material_factors = {0.1, 0.9, 0.3, 10.0},
    });

    MeshInstance *cube_instance_1 = new MeshInstance(cube_mesh, PipelineMatrixManager::Shader::Phong);
    instances.push_back(std::unique_ptr<MeshInstance>(cube_instance_1));
    cube_instance_1->set_uniforms({
        .color = {1.0, 1.0, 1.0, 1.0},
        .model_matrix = glm::rotate(glm::translate(glm::mat4(1.0), {-0.5, -0.8, 0.0}), glm::radians(45.0f), {0, 1, 0}),
        .material_factors = {0.1, 0.7, 0.1, 2.0},
    });
    cube_instance_1->set_texture_index(0);

    MeshInstance *cylinder_instance = new MeshInstance(cylinder_mesh, PipelineMatrixManager::Shader::Phong);
    instances.push_back(std::unique_ptr<MeshInstance>(cylinder_instance));
    cylinder_instance->set_uniforms({
        .color = {1.0, 1.0, 1.0, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {-0.5, 0.3, 0.0}),
        .material_factors = {0.1, 0.7, 0.1, 2.0},
    });
    cylinder_instance->set_texture_index(0);

    MeshInstance *bezier_instance = new MeshInstance(bezier_mesh, PipelineMatrixManager::Shader::Phong);
    instances.push_back(std::unique_ptr<MeshInstance>(bezier_instance));
    bezier_instance->set_uniforms({
        .color = {1.0, 1.0, 1.0, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {0.5, 0, 0}),
        .material_factors = {0.1, 0.7, 0.3, 8.0},
    });
    bezier_instance->set_texture_index(1);

    MeshInstance *sphere_instance_2 = new MeshInstance(sphere_mesh, PipelineMatrixManager::Shader::Phong);
    instances.push_back(std::unique_ptr<MeshInstance>(sphere_instance_2));
    sphere_instance_2->set_uniforms({
        .color = {1.0, 1.0, 1.0, 1.0},
        .model_matrix = glm::translate(glm::mat4(1.0), {0.5, -0.8, 0}),
        .material_factors = {0.1, 0.7, 0.3, 8.0},
    });
    sphere_instance_2->set_texture_index(1);

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
    load_vulkan_extensions(vk_device);
    // PFN_vkCmdPipelineBarrier2KHR g_vkCmdPipelineBarrier2KHR;
    // g_vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(vk_device, "vkCmdPipelineBarrier2KHR"));
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
    INIReader renderer_ini_reader(init_renderer_filepath);

    std::shared_ptr<Camera> camera(createCamera(init_camera_filepath, window));
    trash.push_back(camera);
    std::shared_ptr<Input> input = Input::init(window);
    std::shared_ptr<OrbitControls> controls(new OrbitControls(camera));
    std::shared_ptr<PipelineMatrixManager> pipelines = createPipelineManager(renderer_ini_reader);
    trash.push_back(pipelines);

    // All instances share a uniform buffer
    std::shared_ptr<SharedUniformBuffer> uniform_buffer(new SharedUniformBuffer(vk_physical_device, sizeof(MeshInstanceUniformBlock), 20));
    trash.push_back(uniform_buffer);

    VkDescriptorPool vk_descriptor_pool = createVkDescriptorPool(vk_device, 20, 20 * 2);
    VkDescriptorSetLayout vk_descriptor_set_layout = createVkDescriptorSetLayout(
        vk_device,
        {{.binding = 0,
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
         {.binding = 1,
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
         {.binding = 2,
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
         {.binding = 3,
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
         {.binding = 4,
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
         {.binding = 5,
          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}});

    ShaderConstantsUniformBlock shader_constants = {
        .user_input = {renderer_ini_reader.GetBoolean("renderer", "normals", false), renderer_ini_reader.GetBoolean("renderer", "texcoords", false), 0, 0}};
    VkBuffer shader_constants_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(shader_constants), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(shader_constants_buffer, &shader_constants, sizeof(shader_constants));

    DirectionalLightUniformBlock directional_light = {
        .direction = {0, -1, -1, 0},
        .color = {0.8, 0.8, 0.8, 1.0},
    };
    VkBuffer directional_light_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(directional_light), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(directional_light_buffer, &directional_light, sizeof(directional_light));

    PointLightUniformBlock point_light = {
        .position = {0, 0, 0, 0},
        .color = {1.0, 1.0, 1.0, 1.0},
        .attenuation = {1.0, 0.4, 0.1, 0}};
    VkBuffer point_light_buffer = vklCreateHostCoherentBufferWithBackingMemory(sizeof(point_light), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vklCopyDataIntoHostCoherentBuffer(point_light_buffer, &point_light, sizeof(point_light));

    VkSampler sampler = createSampler(vk_device, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    auto textures = createTextureImages(vk_device, vk_queue, graphics_queue_family, {"wood_texture.dds", "tiles_diffuse.dds"});
    for (auto &&tex : textures)
    {
        std::cout << "main::init_texture " << tex << std::endl
                  << std::flush;
        trash.push_back(tex);
        tex->setSampler(sampler);
    }

    std::cout << "main::createScene " << std::endl
              << std::flush;
    auto mesh_instances = createScene();
    for (size_t i = 0; i < mesh_instances.size(); i++)
    {
        mesh_instances[i]->init_uniforms(vk_device, vk_descriptor_pool, vk_descriptor_set_layout, 1, uniform_buffer->buffer, uniform_buffer->slot(i));
        trash.push_back(mesh_instances[i]->mesh);

        // vklCreateGraphicsPipeline does not allow binding multiple descriptor sets simultaneously
        // thus it's required to hook the scene-static uniforms into every descriptor set
        // See: https://github.com/cg-tuwien/VulkanLaunchpad/issues/30
        VkDescriptorSet descriptor_set = mesh_instances[i]->get_descriptor_set();
        camera->init_uniforms(vk_device, descriptor_set, 0);
        writeDescriptorSetBuffer(vk_device, descriptor_set, 2, shader_constants_buffer, sizeof(shader_constants));
        writeDescriptorSetBuffer(vk_device, descriptor_set, 3, directional_light_buffer, sizeof(directional_light));
        writeDescriptorSetBuffer(vk_device, descriptor_set, 4, point_light_buffer, sizeof(point_light));
        int32_t texture_index = mesh_instances[i]->get_texture_index();
        if (texture_index >= 0)
        {
            std::cout << "main::texture_init_uniforms, texture_index=" << texture_index << std::endl
                      << std::flush;
            textures[texture_index]->init_uniforms(vk_device, descriptor_set, 5);
        }
    }

    std::cout << "main::vklEnablePipelineHotReloading" << std::endl
              << std::flush;
    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);

    while (!glfwWindowShouldClose(window))
    {
        // NOTE: input update need to be called before glfwPollEvents
        input->update();
        glfwPollEvents();

        if (input->isKeyPress(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        if (input->isKeyPress(GLFW_KEY_N))
        {
            shader_constants.user_input.x++;
            shader_constants.user_input.x %= 3;
            vklCopyDataIntoHostCoherentBuffer(shader_constants_buffer, &shader_constants, sizeof(shader_constants));
        }
        if (input->isKeyPress(GLFW_KEY_T))
        {
            shader_constants.user_input.y++;
            shader_constants.user_input.y %= 2;
            vklCopyDataIntoHostCoherentBuffer(shader_constants_buffer, &shader_constants, sizeof(shader_constants));
        }

        pipelines->update();
        controls->update();

        std::cout << "main::vklWaitForNextSwapchainImage" << std::endl
                  << std::flush;
        vklWaitForNextSwapchainImage();

        std::cout << "main::vklStartRecordingCommands" << std::endl
                  << std::flush;
        vklStartRecordingCommands();
        VkCommandBuffer vk_cmd_buffer = vklGetCurrentCommandBuffer();

        std::cout << "main::mesh_loop" << std::endl
                  << std::flush;
        for (auto &&i : mesh_instances)
        {
            std::cout << "main::set_shader" << std::endl
                      << std::flush;
            pipelines->set_shader(i->get_shader());
            VkPipeline vk_selected_pipeline = pipelines->selected();
            std::cout << "main::vklCmdBindPipeline" << std::endl
                      << std::flush;
            vklCmdBindPipeline(vk_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_selected_pipeline);
            std::cout << "main::vklGetLayoutForPipeline" << std::endl
                      << std::flush;
            VkPipelineLayout vk_pipeline_layout = vklGetLayoutForPipeline(vk_selected_pipeline);

            std::cout << "main::bind_uniforms" << std::endl
                      << std::flush;
            i->bind_uniforms(vk_cmd_buffer, vk_pipeline_layout);
            std::cout << "main::mesh_bind" << std::endl
                      << std::flush;
            i->mesh->bind(vk_cmd_buffer);
            std::cout << "main::mesh_draw" << std::endl
                      << std::flush;
            i->mesh->draw(vk_cmd_buffer);
        }

        std::cout << "main::vklEndRecordingCommands" << std::endl
                  << std::flush;
        vklEndRecordingCommands();
        std::cout << "main::vklPresentCurrentSwapchainImage" << std::endl
                  << std::flush;
        vklPresentCurrentSwapchainImage();

        if (cmdline_args.run_headless)
        {
            std::cout << "main::vklGetCurrentSwapChainImageIndex" << std::endl
                      << std::flush;
            uint32_t idx = vklGetCurrentSwapChainImageIndex();
            std::string screenshot_filename = "screenshot";
            if (cmdline_args.set_filename)
                screenshot_filename = cmdline_args.filename;

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            std::cout << "main::gcgSaveScreenshot" << std::endl
                      << std::flush;
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
    vklDestroyHostCoherentBufferAndItsBackingMemory(shader_constants_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(directional_light_buffer);
    vklDestroyHostCoherentBufferAndItsBackingMemory(point_light_buffer);
    vklDestroyDeviceLocalImageAndItsBackingMemory(swapchain_depth_attachment.image);
    for (auto &&i : trash)
    {
        i->destroy(vk_device);
    }
    vkDestroySampler(vk_device, sampler, nullptr);
    gcgDestroyFramework();
    vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);
    vkDestroyDevice(vk_device, nullptr);
    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
