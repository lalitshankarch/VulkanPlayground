#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#ifdef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif
#include <spdlog/spdlog.h>

void check(auto val, const char *msg)
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
    vkb::Instance vkb_instance{};
    VkSurfaceKHR surface{};
    vkb::Device vkb_device{};
    VkQueue queue{};
    vkb::Swapchain vkb_swapchain{};
    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};
    VkCommandPool commandPool{};
    VkCommandBuffer commandBuffer{};
    VkFence renderFence{};
    VkSemaphore renderSemaphore{}, swapchainSemaphore{};

    void init()
    {
        initGLFW();
        initVulkan();
        initSwapchain();
        initFrameData();
    }
    void initGLFW()
    {
        spdlog::info("Initialize GLFW");

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "HelloTriangle", nullptr, nullptr);
        check(window, "GLFW: Failed to create window");
    }
    void initVulkan()
    {
        spdlog::info("Initialize Vulkan using VkBootstrap");

        vkb::InstanceBuilder instance_builder{};
        instance_builder = instance_builder.set_app_name("HelloTriangle").require_api_version(1, 3, 0);
#ifndef NDEBUG
        instance_builder = instance_builder.request_validation_layers().use_default_debug_messenger();
#endif
        auto instance_builder_ret = instance_builder.build();
        check(instance_builder_ret, "Vulkan: Failed to create instance");
        vkb_instance = instance_builder_ret.value();

        check(
            glfwCreateWindowSurface(
                vkb_instance.instance, window, nullptr, &surface) == VK_SUCCESS,
            "GLFW: Failed to create window surface");

        VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        vkb::PhysicalDeviceSelector phys_dev_selector{vkb_instance};
        auto phys_dev_selector_ret = phys_dev_selector.set_minimum_version(1, 3).set_required_features_13(features13).set_surface(surface).select();
        check(phys_dev_selector_ret, "Vulkan: Failed to select physical device");

        vkb::DeviceBuilder dev_builder{phys_dev_selector_ret.value()};
        auto dev_builder_ret = dev_builder.build();
        check(phys_dev_selector_ret, "Vulkan: Failed to create logical device");
        vkb_device = dev_builder_ret.value();

        auto get_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        check(get_queue_ret, "Vulkan: Failed to get graphics queue");
    }
    void initSwapchain()
    {
        spdlog::info("Initialize swapchain");

        int width{}, height{};
        glfwGetFramebufferSize(window, &width, &height);

        vkb::SwapchainBuilder swapchain_builder{vkb_device};
        auto swapchain_builder_ret = swapchain_builder
                                         .set_desired_format(VkSurfaceFormatKHR{.format = VK_FORMAT_B8G8R8A8_UNORM})
                                         .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                                         .set_desired_extent(width, height)
                                         .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                         .build();
        check(swapchain_builder_ret, "Vulkan: Failed to build swapchain");
        vkb_swapchain = swapchain_builder_ret.value();

        spdlog::info("Get swapchain images and image views");

        swapchainImages = vkb_swapchain.get_images().value();
        swapchainImageViews = vkb_swapchain.get_image_views().value();
    }
    void initFrameData()
    {
        spdlog::info("Initialize command pool and buffer");

        VkCommandPoolCreateInfo command_pool_ci{};
        command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_ci.queueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
        command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        check(
            vkCreateCommandPool(
                vkb_device.device, &command_pool_ci, nullptr, &commandPool) == VK_SUCCESS,
            "Vulkan: Failed to create command pool");

        VkCommandBufferAllocateInfo command_buffer_ai{};
        command_buffer_ai.commandBufferCount = 1;
        command_buffer_ai.commandPool = commandPool;
        command_buffer_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        check(
            vkAllocateCommandBuffers(vkb_device.device, &command_buffer_ai, &commandBuffer) == VK_SUCCESS,
            "Vulkan: Failed to allocate command buffer");

        spdlog::info("Create render fence");

        VkFenceCreateInfo fence_ci{};
        fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        check(vkCreateFence(vkb_device.device, &fence_ci, nullptr, &renderFence) == VK_SUCCESS,
              "Vulkan: Failed to create render fence");

        spdlog::info("Create swapchain and render semaphore");

        VkSemaphoreCreateInfo semaphore_ci{};
        semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        check(vkCreateSemaphore(vkb_device.device, &semaphore_ci, nullptr, &renderSemaphore) == VK_SUCCESS,
              "Vulkan: Failed to create render semaphore");
        check(vkCreateSemaphore(vkb_device.device, &semaphore_ci, nullptr, &swapchainSemaphore) == VK_SUCCESS,
              "Vulkan: Failed to create swapchain semaphore");
    }
    void renderLoop()
    {
        spdlog::info("Begin render loop");

        VkCommandBufferBeginInfo command_buffer_bi{};
        command_buffer_bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // Wait until GPU has finished rendering the last frame
            vkWaitForFences(vkb_device.device, 1, &renderFence, VK_TRUE, UINT64_MAX);
            vkResetFences(vkb_device.device, 1, &renderFence);

            // Request image from swapchain
            uint32_t swapchain_img_idx;
            vkAcquireNextImageKHR(
                vkb_device.device, vkb_swapchain.swapchain, UINT64_MAX, swapchainSemaphore, nullptr,
                &swapchain_img_idx);

            // Start recording commands
            vkResetCommandBuffer(commandBuffer, 0);
            vkBeginCommandBuffer(commandBuffer, &command_buffer_bi);

            VkImageMemoryBarrier2 image_barrier{};
            image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
            image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
            image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            image_barrier.image = swapchainImages[swapchain_img_idx];
            VkImageSubresourceRange sub_image{};
            sub_image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
            sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;
            image_barrier.subresourceRange = sub_image;
            VkDependencyInfo dependency_i{};
            dependency_i.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency_i.imageMemoryBarrierCount = 1;
            dependency_i.pImageMemoryBarriers = &image_barrier;

            vkCmdPipelineBarrier2(commandBuffer, &dependency_i);

            VkClearColorValue clear_value = {1.0f, 0.0f, 0.0f, 1.0f};
            vkCmdClearColorImage(commandBuffer, swapchainImages[swapchain_img_idx], VK_IMAGE_LAYOUT_GENERAL,
                                 &clear_value, 1, &sub_image);

            image_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            vkCmdPipelineBarrier2(commandBuffer, &dependency_i);

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &swapchainSemaphore;
            VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submit_info.pWaitDstStageMask = &wait_stage;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &commandBuffer;
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &renderSemaphore;

            // Submit command buffer
            vkQueueSubmit(vkb_device.get_queue(vkb::QueueType::graphics).value(), 1, &submit_info, renderFence);

            // Present the image
            VkPresentInfoKHR present_info{};
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &renderSemaphore;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &vkb_swapchain.swapchain;
            present_info.pImageIndices = &swapchain_img_idx;
            present_info.pResults = nullptr;

            vkQueuePresentKHR(vkb_device.get_queue(vkb::QueueType::graphics).value(), &present_info);
        }
    }

    void destroyFrameData()
    {
        vkDestroySemaphore(vkb_device.device, swapchainSemaphore, nullptr);
        vkDestroySemaphore(vkb_device.device, renderSemaphore, nullptr);
        vkDestroyFence(vkb_device.device, renderFence, nullptr);
        vkDestroyCommandPool(vkb_device.device, commandPool, nullptr);
    }
    void destroySwapchain()
    {
        for (auto &imageView : swapchainImageViews)
        {
            vkDestroyImageView(vkb_device.device, imageView, nullptr);
        }
        vkb::destroy_swapchain(vkb_swapchain);
    }
    void cleanup()
    {
        spdlog::info("Cleanup");
        vkDeviceWaitIdle(vkb_device.device);
        destroyFrameData();
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
        HelloTriangleApp app;
        app.run();
    }
    catch (std::exception e)
    {
        spdlog::error(e.what());
    }
}