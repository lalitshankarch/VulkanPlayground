#include <cstddef>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#ifdef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif
#include <spdlog/spdlog.h>
#include <VkBootstrap.h>

class HelloTriangleApp
{
public:
    void run()
    {
        init();
        renderLoop();
        cleanup();
    }

private:
    GLFWwindow *window{};
    const uint32_t WIDTH = 640, HEIGHT = 480;
    uint32_t fb_width{}, fb_height{};
    vkb::Instance vkb_instance{};
    VkSurfaceKHR surface{};
    vkb::Device vkb_device{};
    VkQueue graphics_queue{};
    vkb::Swapchain vkb_swapchain{};
    std::vector<VkImageView> swapchain_image_views{};
    VkPipelineShaderStageCreateInfo shader_stage_cis[2]{};
    VkPipeline graphics_pipeline{};
    VkPipelineLayout pipeline_layout{};
    VkRenderPass render_pass{};
    VkCommandPool command_pool{};
    std::vector<VkCommandBuffer> command_buffers{};
    std::vector<VkFramebuffer> frame_buffers{};

    inline void check(auto val, const char *msg)
    {
#ifdef NDEBUG
        (void)val;
        (void)msg;
#else
        if (!val)
        {
            throw std::runtime_error(msg);
        }
#endif
    }
    void initGLFW()
    {
        spdlog::info("GLFW: Initialize");

        check(glfwInit(), "GLFW: Failed to initialize");

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "HelloTriangle", nullptr, nullptr);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        fb_width = static_cast<uint32_t>(width);
        fb_height = static_cast<uint32_t>(height);
    }
    void initVulkan()
    {
        spdlog::info("VkBootstrap: Initialize");

        vkb::InstanceBuilder vkb_inst_buildr{};
#ifdef NDEBUG
        auto inst_ret = vkb_inst_buildr.set_app_name("HelloTriangle").build();
#else
        auto inst_ret = vkb_inst_buildr.set_app_name("HelloTriangle")
                            .enable_validation_layers()
                            .use_default_debug_messenger()
                            .build();
#endif
        check(inst_ret, "Vulkan: Failed to create instance");
        vkb_instance = inst_ret.value();

        check(glfwCreateWindowSurface(vkb_instance.instance, window, nullptr, &surface) == VK_SUCCESS,
              "Vulkan: Failed to create window surface");

        vkb::PhysicalDeviceSelector vkb_phys_dev_selectr{vkb_instance};
        auto phys_dev_ret = vkb_phys_dev_selectr.set_minimum_version(1, 0).set_surface(surface).select();
        check(phys_dev_ret, "Vulkan: Failed to select physical device");

        vkb::DeviceBuilder vkb_dev_buildr{phys_dev_ret.value()};
        auto dev_ret = vkb_dev_buildr.build();
        check(dev_ret, "Vulkan: Failed to create logical device");
        vkb_device = dev_ret.value();

        auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        check(graphics_queue_ret, "Vulkan: Failed to get graphics queue");
        graphics_queue = graphics_queue_ret.value();
    }
    void createSwapchain()
    {
        spdlog::info("Create swapchain");

        VkSurfaceFormatKHR surf_format{
            .format = VK_FORMAT_B8G8R8A8_UNORM,
        };

        vkb::SwapchainBuilder vkb_swapchain_buildr{vkb_device};
        auto swapchain_ret = vkb_swapchain_buildr
                                 .set_desired_format(surf_format)
                                 .set_desired_min_image_count(3)
                                 .build();
        check(swapchain_ret, "Vulkan: Failed to create swapchain");
        vkb_swapchain = swapchain_ret.value();

        swapchain_image_views = vkb_swapchain.get_image_views().value();
    }
    std::vector<char> readFile(const std::string &path)
    {
        std::ifstream file(path, std::ios_base::binary);
        check(file.is_open(), "Failed to open file");

        file.seekg(0, std::ios_base::end);
        size_t file_size = file.tellg();
        check(file_size, "File is empty");
        file.seekg(std::ios_base::beg);

        std::vector<char> buffer(file_size);
        file.read(buffer.data(), file_size);
        check(buffer.size() == file_size, "Failed to read file completely");

        return buffer;
    }
    VkShaderModule loadShaderModule(const std::string &path)
    {
        spdlog::info("Load shader module: {}", path);

        std::vector<char> code = readFile(path);

        VkShaderModuleCreateInfo shader_module_ci{};
        shader_module_ci.codeSize = code.size();
        shader_module_ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
        shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        VkShaderModule shader_module{};
        check(vkCreateShaderModule(vkb_device.device, &shader_module_ci, nullptr, &shader_module) == VK_SUCCESS,
              "Failed to create shader module");

        return shader_module;
    }
    void setupShaderStage()
    {
        spdlog::info("Setup shaders");

        VkShaderModule shader_modules[2] = {loadShaderModule("shaders/vert.spv"),
                                            loadShaderModule("shaders/frag.spv")};

        for (int i = 0; i < 2; i++)
        {
            shader_stage_cis[i].module = shader_modules[i];
            shader_stage_cis[i].pName = "main";
            shader_stage_cis[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        }

        shader_stage_cis[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader_stage_cis[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    void createRenderPass()
    {
        VkAttachmentDescription attachment_desc{
            .format = vkb_swapchain.image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

        VkAttachmentReference color_attachment_ref{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass_desc{
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_ref,
        };

        VkRenderPassCreateInfo render_pass_ci{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachment_desc,
            .subpassCount = 1,
            .pSubpasses = &subpass_desc};

        check(
            vkCreateRenderPass(vkb_device.device, &render_pass_ci, nullptr, &render_pass) == VK_SUCCESS,
            "Vulkan: Failed to create render pass");

        frame_buffers.resize(vkb_swapchain.image_count);

        VkFramebufferCreateInfo frame_buffer_ci{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .width = fb_width,
            .height = fb_height,
            .layers = 1};

        for (int i = 0; i < frame_buffers.size(); i++)
        {
            frame_buffer_ci.pAttachments = &swapchain_image_views[i],

            check(
                vkCreateFramebuffer(vkb_device.device, &frame_buffer_ci, nullptr, &frame_buffers[i]) == VK_SUCCESS,
                "Vulkan: Failed to create frame buffer");
        }
    }
    void createGraphicsPipeline()
    {
        spdlog::info("Create graphics pipeline");

        setupShaderStage();

        VkPipelineVertexInputStateCreateInfo vertex_input_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

        VkViewport viewport{.width = static_cast<float>(fb_width), .height = static_cast<float>(fb_height)};

        VkRect2D scissor{};
        scissor.extent = {
            .width = fb_width,
            .height = fb_height,
        };

        VkPipelineViewportStateCreateInfo viewport_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor};

        VkPipelineInputAssemblyStateCreateInfo input_assembly_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

        VkPipelineRasterizationStateCreateInfo rasterization_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .lineWidth = 1.0f};

        VkPipelineMultisampleStateCreateInfo multisample_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

        VkPipelineColorBlendAttachmentState color_blend_attachment_state{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

        VkPipelineColorBlendStateCreateInfo color_blend_sci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment_state};

        VkPipelineLayoutCreateInfo pipeline_layout_ci{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };

        check(vkCreatePipelineLayout(vkb_device.device, &pipeline_layout_ci, nullptr, &pipeline_layout) == VK_SUCCESS,
              "Vulkan: Failed to create pipeline layout");

        createRenderPass();

        VkGraphicsPipelineCreateInfo graphics_pipeline_ci{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shader_stage_cis,
            .pVertexInputState = &vertex_input_sci,
            .pInputAssemblyState = &input_assembly_sci,
            .pViewportState = &viewport_sci,
            .pRasterizationState = &rasterization_sci,
            .pMultisampleState = &multisample_sci,
            .pColorBlendState = &color_blend_sci,
            .layout = pipeline_layout,
            .renderPass = render_pass};

        check(
            vkCreateGraphicsPipelines(
                vkb_device.device, nullptr, 1, &graphics_pipeline_ci, nullptr, &graphics_pipeline) == VK_SUCCESS,
            "Vulkan: Failed to create graphics pipeline");
    }
    void createCommandBuffers()
    {
        VkCommandPoolCreateInfo command_pool_ci{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value()};

        check(
            vkCreateCommandPool(vkb_device.device, &command_pool_ci, nullptr, &command_pool) == VK_SUCCESS,
            "Vulkan: Failed to create command pool");

        command_buffers.resize(vkb_swapchain.image_count);

        VkCommandBufferAllocateInfo command_buffer_ai{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .commandBufferCount = static_cast<uint32_t>(command_buffers.size())};

        check(
            vkAllocateCommandBuffers(vkb_device.device, &command_buffer_ai, command_buffers.data()) == VK_SUCCESS,
            "Vulkan: Failed to allocate command buffers");
    }
    void init()
    {
        initGLFW();
        initVulkan();
        createSwapchain();
        createGraphicsPipeline();
        createCommandBuffers();
    }
    void renderLoop()
    {
        spdlog::info("Enter render loop");

        uint32_t img_idx{};

        VkFenceCreateInfo fence_ci{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        VkFence swapchain_fence{}, render_fence{};
        check(
            vkCreateFence(vkb_device.device, &fence_ci, nullptr, &swapchain_fence) == VK_SUCCESS,
            "Vulkan: Failed to create swapchain fence");
        check(
            vkCreateFence(vkb_device.device, &fence_ci, nullptr, &render_fence) == VK_SUCCESS,
            "Vulkan: Failed to create render fence");

        VkCommandBufferBeginInfo command_buffer_bi{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

        VkClearValue clear_value{.color = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderPassBeginInfo render_pass_bi{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .clearValueCount = 1,
            .pClearValues = &clear_value,
        };
        render_pass_bi.renderArea.extent.width = fb_width;
        render_pass_bi.renderArea.extent.height = fb_height;

        VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1};

        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &vkb_swapchain.swapchain};

        // Record commands in advance
        for (int i = 0; i < frame_buffers.size(); i++)
        {
            vkResetCommandBuffer(command_buffers[i], 0);
            vkBeginCommandBuffer(command_buffers[i], &command_buffer_bi);
            render_pass_bi.framebuffer = frame_buffers[i];
            vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
            vkCmdBeginRenderPass(command_buffers[i], &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(command_buffers[i]);
            vkEndCommandBuffer(command_buffers[i]);
        }

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // Wait until all commands have executed on graphics queue
            vkWaitForFences(vkb_device.device, 1, &render_fence, VK_TRUE, 1000000000);
            vkResetFences(vkb_device.device, 1, &swapchain_fence);
            vkAcquireNextImageKHR(
                vkb_device.device, vkb_swapchain.swapchain, 1000000000, VK_NULL_HANDLE, swapchain_fence, &img_idx);

            // Wait until next image is acquired
            vkWaitForFences(vkb_device.device, 1, &swapchain_fence, VK_TRUE, 1000000000);
            vkResetFences(vkb_device.device, 1, &render_fence);
            submit_info.pCommandBuffers = &command_buffers[img_idx];
            vkQueueSubmit(graphics_queue, 1, &submit_info, render_fence);

            present_info.pImageIndices = &img_idx,
            vkQueuePresentKHR(graphics_queue, &present_info);
        }

        vkDeviceWaitIdle(vkb_device.device);
        vkDestroyFence(vkb_device.device, swapchain_fence, nullptr);
        vkDestroyFence(vkb_device.device, render_fence, nullptr);
    }
    void destroySwapchain()
    {
        spdlog::info("Destroy swapchain");

        for (const auto &frame_buffer : frame_buffers)
        {
            vkDestroyFramebuffer(vkb_device.device, frame_buffer, nullptr);
        }

        for (const auto &image_view : swapchain_image_views)
        {
            vkDestroyImageView(vkb_device.device, image_view, nullptr);
        }

        vkb::destroy_swapchain(vkb_swapchain);
    }
    void destroyGraphicsPipeline()
    {
        spdlog::info("Destroy graphics pipeline");

        vkDestroyPipeline(vkb_device, graphics_pipeline, nullptr);
        vkDestroyRenderPass(vkb_device.device, render_pass, nullptr);
        vkDestroyPipelineLayout(vkb_device.device, pipeline_layout, nullptr);

        for (const auto &shader_stage_ci : shader_stage_cis)
        {
            vkDestroyShaderModule(vkb_device.device, shader_stage_ci.module, nullptr);
        }
    }
    void cleanup()
    {
        spdlog::info("Cleanup");

        vkDestroyCommandPool(vkb_device.device, command_pool, nullptr);
        destroyGraphicsPipeline();
        destroySwapchain();
        vkb::destroy_device(vkb_device);
        vkDestroySurfaceKHR(vkb_instance.instance, surface, nullptr);
        vkb::destroy_instance(vkb_instance);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

#ifdef _WIN32
#ifdef NDEBUG
#include <windows.h>
#define main() WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
#endif

int main()
{
    try
    {
        HelloTriangleApp app{};
        app.run();

        return EXIT_SUCCESS;
    }
    catch (std::exception e)
    {
        spdlog::error(e.what());

        return EXIT_FAILURE;
    }
}