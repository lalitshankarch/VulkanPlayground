#pragma once

#include <memory>
#include "lve_window.hpp"
#include "lve_pipeline.hpp"
#include "lve_device.hpp"
#include "lve_swap_chain.hpp"

namespace lve
{
    class FirstApp
    {
    public:
        static constexpr int WIDTH = 640, HEIGHT = 480;
        FirstApp();
        ~FirstApp();
        void run();

        FirstApp(const FirstApp &) = delete;
        FirstApp &operator=(const FirstApp &) = delete;

    private:
        void createPipelineLayout();
        void createPipeline();
        void createCommandBuffers();
        void drawFrame();

        LveWindow lveWindow{WIDTH, HEIGHT, "Hello, Vulkan!"};
        LveDevice lveDevice{lveWindow};
        LveSwapChain lveSwapchain{lveDevice, lveWindow.getExtent()};
        std::unique_ptr<LvePipeline> lvePipeline;
        VkPipelineLayout pipelineLayout{};
        std::vector<VkCommandBuffer> commandBuffers{};
    };
}