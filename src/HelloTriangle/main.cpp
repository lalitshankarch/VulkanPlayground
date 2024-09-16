#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <fstream>

const uint32_t width = 640, height = 480;

// Throws std::runtime_error on nullptr or falsy values
static void check(const auto &ptr, const std::string &msg)
{
#ifdef NDEBUG
    (void)ptr;
    (void)msg;
#else
    if (!ptr)
    {
        spdlog::error(msg);
        throw std::runtime_error(msg);
    }
#endif
}

#ifdef _WIN32
#ifdef NDEBUG
#include <windows.h>
#define main() WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
#endif

int main()
{
    vkb::InstanceBuilder builder{};
#ifdef NDEBUG
    auto inst_ret =
        builder.set_app_name("HelloTriangle").request_validation_layers().build();
#else
    auto inst_ret = builder.set_app_name("HelloTriangle")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();
#endif
    check(inst_ret, "Vulkan: Failed to create instance");
    vkb::Instance vkb_inst = inst_ret.value();

    check(glfwInit(), "GLFW: Failed to initialize");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(
        width, height, "VulkanPlayground: HelloTriangle", nullptr, nullptr);
    check(window, "GLFW: Failed to create window");

    vk::SurfaceKHR surface{};
    auto glfw_ret =
        glfwCreateWindowSurface(vkb_inst.instance, window, nullptr,
                                reinterpret_cast<VkSurfaceKHR *>(&surface));
    check(glfw_ret == VK_SUCCESS, "GLFW: Failed to create window surface");

    vkb::PhysicalDeviceSelector phys_dev_selector{vkb_inst};
    auto phys_ret = phys_dev_selector.set_surface(surface)
                        .set_minimum_version(1, 2)
                        .select();
    check(phys_ret, "Vulkan: Failed to select physical device");

    vkb::DeviceBuilder dev_builder{phys_ret.value()};
    auto dev_ret = dev_builder.build();
    check(dev_ret, "Vulkan: Failed to create logical device");
    vkb::Device vkb_dev = dev_ret.value();
    vk::Device device = vkb_dev.device;

    auto gr_queue_ret = vkb_dev.get_queue(vkb::QueueType::graphics);
    check(gr_queue_ret, "Vulkan: Failed to get graphics queue");
    vk::Queue graphics_queue = gr_queue_ret.value();

    vkb::SwapchainBuilder swapchain_builder{vkb_dev};
    auto swap_ret = swapchain_builder.build();
    check(swap_ret, "Vulkan: Failed to create swapchain");
    vkb::Swapchain vkb_swapchain = swap_ret.value();

    auto images = vkb_swapchain.get_images().value();
    auto image_views = vkb_swapchain.get_image_views().value();

    vk::CommandPoolCreateInfo create_info{};
    create_info.queueFamilyIndex = vkb_dev.get_queue_index(vkb::QueueType::graphics).value();
    create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::CommandPool command_pool{};
    check(device.createCommandPool(&create_info, nullptr, &command_pool) == vk::Result::eSuccess, "Failed to create command pool");

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandBufferCount = 1;
    alloc_info.commandPool = command_pool;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;

    vk::CommandBuffer command_buffer{};
    check(device.allocateCommandBuffers(&alloc_info, &command_buffer) == vk::Result::eSuccess, "Failed to allocate command buffer");

    vk::AttachmentDescription color_attachment{};
    color_attachment.format = static_cast<vk::Format>(vkb_swapchain.image_format);
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass_desc{};
    subpass_desc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_ref;

    vk::RenderPassCreateInfo rp_create_info{};
    rp_create_info.attachmentCount = 1;
    rp_create_info.pAttachments = &color_attachment;
    rp_create_info.subpassCount = 1;
    rp_create_info.pSubpasses = &subpass_desc;

    vk::RenderPass render_pass = device.createRenderPass(rp_create_info);
    check(render_pass, "Failed to create render pass");

    vk::FramebufferCreateInfo fb_create_info{};
    fb_create_info.renderPass = render_pass;
    fb_create_info.attachmentCount = 1;
    fb_create_info.width = width;
    fb_create_info.height = height;
    fb_create_info.layers = 1;

    std::vector<vk::Framebuffer> framebuffers{};
    for (vk::ImageView image_view : image_views)
    {
        fb_create_info.pAttachments = &image_view;
        vk::Framebuffer frame_buffer = device.createFramebuffer(fb_create_info);
        check(frame_buffer, "Failed to create framebuffer");
        framebuffers.emplace_back(frame_buffer);
    }

    vk::FenceCreateInfo fc_create_info{};
    fc_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

    vk::Fence fence = device.createFence(fc_create_info);
    check(fence, "Failed to create fence");

    vk::SemaphoreCreateInfo sm_create_info{};

    vk::Semaphore render_semaphore{}, present_semaphore{};
    render_semaphore = device.createSemaphore(sm_create_info);
    check(render_semaphore, "Failed to create render semaphore");
    present_semaphore = device.createSemaphore(sm_create_info);
    check(present_semaphore, "Failed to create present semaphore");

    vk::CommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        device.waitForFences(1, &fence, true, UINT64_MAX);
        device.resetFences(1, &fence);

        uint32_t swapchain_img_idx{};
        device.acquireNextImageKHR(vkb_swapchain.swapchain, UINT64_MAX, present_semaphore, VK_NULL_HANDLE, &swapchain_img_idx);

        command_buffer.reset();

        command_buffer.begin(cmd_begin_info);

        vk::ClearValue clear_value{};
        clear_value.color = {0.39f, 0.58f, 0.93f, 1.0f};

        vk::RenderPassBeginInfo rp_begin_info{};
        rp_begin_info.renderPass = render_pass;
        rp_begin_info.renderArea.extent.width = width;
        rp_begin_info.renderArea.extent.height = height;
        rp_begin_info.framebuffer = framebuffers[swapchain_img_idx];
        rp_begin_info.clearValueCount = 1;
        rp_begin_info.pClearValues = &clear_value;

        command_buffer.beginRenderPass(&rp_begin_info, vk::SubpassContents::eInline);
        command_buffer.endRenderPass();
        command_buffer.end();

        vk::SubmitInfo submit_info{};
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &present_semaphore;
        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        submit_info.pWaitDstStageMask = &wait_stage;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &render_semaphore;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        graphics_queue.submit(1, &submit_info, fence);

        vk::PresentInfoKHR present_info{};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = reinterpret_cast<vk::SwapchainKHR *>(&vkb_swapchain.swapchain);
        present_info.pImageIndices = &swapchain_img_idx;

        graphics_queue.presentKHR(&present_info);
    }

    device.waitIdle();
    for (auto &frame_buffer : framebuffers)
    {
        device.destroyFramebuffer(frame_buffer);
    }
    device.destroySemaphore(present_semaphore);
    device.destroySemaphore(render_semaphore);
    device.destroyFence(fence);
    device.destroyRenderPass(render_pass);
    device.destroyCommandPool(command_pool);
    vkb_swapchain.destroy_image_views(image_views);
    vkb::destroy_swapchain(vkb_swapchain);
    vkb::destroy_device(vkb_dev);
    vkb::destroy_surface(vkb_inst, surface);
    vkb::destroy_instance(vkb_inst);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
