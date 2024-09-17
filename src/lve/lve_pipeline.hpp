#pragma once

#include "lve_device.hpp"
#include <string>
#include <vector>

namespace lve
{
    struct PipelineConfigInfo
    {
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineInputAssemblyStateCreateInfo inputAssemblySci{};
        VkPipelineRasterizationStateCreateInfo rasterizationSci{};
        VkPipelineMultisampleStateCreateInfo multisampleSci{};
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        VkPipelineColorBlendStateCreateInfo colorBlendSci{};
        VkPipelineDepthStencilStateCreateInfo depthStenciSci{};
        VkPipelineLayout layout{};
        VkRenderPass renderPass{};
        uint32_t subpass{};
    };
    class LvePipeline
    {
    public:
        LvePipeline(
            LveDevice &device,
            const std::string &vertFilePath,
            const std::string &fragFilePath,
            const PipelineConfigInfo &configInfo);
        ~LvePipeline();
        LvePipeline(const LvePipeline &) = delete;
        void operator=(const LvePipeline &) = delete;
        void bind(VkCommandBuffer commandBuffer);
        static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

    private:
        static std::vector<char> readFile(const std::string &filePath);
        void createGraphicsPipeline(
            const std::string &vertFilePath,
            const std::string &fragFilePath,
            const PipelineConfigInfo &configInfo);

        void createShaderModule(const std::vector<char> &code, VkShaderModule &shaderModule);

        LveDevice &device;
        VkPipeline graphicsPipeline{};
        VkShaderModule vertShaderModule{};
        VkShaderModule fragShaderModule{};
    };
}