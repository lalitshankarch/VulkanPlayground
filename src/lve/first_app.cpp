#include "first_app.hpp"
#include <stdexcept>

namespace lve
{
    FirstApp::FirstApp()
    {
        createPipelineLayout();
        createPipeline();
        createCommandBuffers();
    }
    FirstApp::~FirstApp()
    {
        vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    }

    void FirstApp::run()
    {
        while (!lveWindow.shouldClose())
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(lveDevice.device());
    }
    void FirstApp::createPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCi{};
        pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutCi, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan: Failed to create pipeline layout");
        }
    }

    void FirstApp::createPipeline()
    {
        auto pipelineConfig = LvePipeline::defaultPipelineConfigInfo(lveSwapchain.width(), lveSwapchain.height());
        pipelineConfig.renderPass = lveSwapchain.getRenderPass();
        pipelineConfig.layout = pipelineLayout;
        lvePipeline = std::make_unique<LvePipeline>(
            lveDevice,
            "src/lve/shaders/simple_vert.spv",
            "src/lve/shaders/simple_frag.spv",
            pipelineConfig);
    }

    void FirstApp::createCommandBuffers()
    {
        commandBuffers.resize(lveSwapchain.imageCount());

        VkCommandBufferAllocateInfo commandBufferAi{};
        commandBufferAi.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        commandBufferAi.commandPool = lveDevice.getCommandPool();
        commandBufferAi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        if (vkAllocateCommandBuffers(lveDevice.device(), &commandBufferAi, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan: Failed to allocate command buffers");
        }

        for (int i = 0; i < commandBuffers.size(); i++)
        {
            VkCommandBufferBeginInfo commandBufferBi{};
            commandBufferBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &commandBufferBi) != VK_SUCCESS)
            {
                throw std::runtime_error("Vulkan: Failed to begin recording command buffer");
            }

            VkRenderPassBeginInfo renderPassBi{};
            renderPassBi.renderPass = lveSwapchain.getRenderPass();
            renderPassBi.framebuffer = lveSwapchain.getFrameBuffer(i);
            renderPassBi.renderArea.extent = lveSwapchain.getSwapChainExtent();
            renderPassBi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            std::vector<VkClearValue> clearValues(2);
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};
            renderPassBi.clearValueCount = 2;
            renderPassBi.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassBi, VK_SUBPASS_CONTENTS_INLINE);
            lvePipeline->bind(commandBuffers[i]);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);
            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Vulkan: Failed to record command buffer");
            }
        }
    }

    void FirstApp::drawFrame()
    {
        uint32_t imageIndex{};
        auto result = lveSwapchain.acquireNextImage(&imageIndex);
        if (result != VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Vulkan: Failed to acquire next image");
        }
        result = lveSwapchain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan: Failed to present swapchain image");
        }
    }
}