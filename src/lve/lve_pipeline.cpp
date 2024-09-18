#include "lve_pipeline.hpp"
#include <fstream>
#include <stdexcept>

namespace lve
{
    LvePipeline::LvePipeline(
        LveDevice &device,
        const std::string &vertFilePath,
        const std::string &fragFilePath,
        const PipelineConfigInfo &configInfo) : device{device}
    {
        createGraphicsPipeline(vertFilePath, fragFilePath, configInfo);
    }

    LvePipeline::~LvePipeline()
    {
        vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
        vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
        vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
    }
    void LvePipeline::bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    std::vector<char> LvePipeline::readFile(const std::string &filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("LvePipeline: Failed to open file");
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void LvePipeline::createGraphicsPipeline(
        const std::string &vertFilePath,
        const std::string &fragFilePath,
        const PipelineConfigInfo &configInfo)
    {
        auto vertCode = readFile(vertFilePath);
        auto fragCode = readFile(fragFilePath);

        createShaderModule(vertCode, vertShaderModule);
        createShaderModule(fragCode, fragShaderModule);

        VkPipelineShaderStageCreateInfo shaderStages[2]{};
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        // Hardcoding data, so keep everything default
        VkPipelineVertexInputStateCreateInfo vertexIsci{};
        vertexIsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineViewportStateCreateInfo viewportSci{};
        viewportSci.pScissors = &configInfo.scissor;
        viewportSci.pViewports = &configInfo.viewport;
        viewportSci.scissorCount = 1;
        viewportSci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportSci.viewportCount = 1;

        VkGraphicsPipelineCreateInfo graphicsPipeline_ci{};
        graphicsPipeline_ci.pStages = shaderStages;
        graphicsPipeline_ci.pViewportState = &viewportSci;
        graphicsPipeline_ci.pInputAssemblyState = &configInfo.inputAssemblySci;
        graphicsPipeline_ci.pColorBlendState = &configInfo.colorBlendSci;
        graphicsPipeline_ci.pDepthStencilState = &configInfo.depthStenciSci;
        graphicsPipeline_ci.pMultisampleState = &configInfo.multisampleSci;
        graphicsPipeline_ci.pRasterizationState = &configInfo.rasterizationSci;
        graphicsPipeline_ci.pVertexInputState = &vertexIsci;
        graphicsPipeline_ci.layout = configInfo.layout;
        graphicsPipeline_ci.renderPass = configInfo.renderPass;
        graphicsPipeline_ci.subpass = configInfo.subpass;
        graphicsPipeline_ci.stageCount = 2;
        graphicsPipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        if (vkCreateGraphicsPipelines(device.device(), nullptr, 1, &graphicsPipeline_ci, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan: Failed to create graphics pipeline");
        }
    }

    void LvePipeline::createShaderModule(const std::vector<char> &code, VkShaderModule &shaderModule)
    {
        VkShaderModuleCreateInfo shader_module_ci{};
        shader_module_ci.codeSize = code.size();
        shader_module_ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
        shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        if (vkCreateShaderModule(device.device(), &shader_module_ci, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Vulkan: Failed to create shader module");
        }
    }

    PipelineConfigInfo LvePipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height)
    {
        PipelineConfigInfo configInfo;

        configInfo.inputAssemblySci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.inputAssemblySci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Don't adjust viewport or scissor
        configInfo.viewport.width = static_cast<float>(width);
        configInfo.viewport.height = static_cast<float>(height);
        configInfo.viewport.maxDepth = 1.0f;

        configInfo.scissor.extent.width = width;
        configInfo.scissor.extent.height = height;

        // Polygons drawn filled with backface culling
        configInfo.rasterizationSci.cullMode = VK_CULL_MODE_BACK_BIT;
        configInfo.rasterizationSci.lineWidth = 1.0f;
        configInfo.rasterizationSci.polygonMode = VK_POLYGON_MODE_FILL;
        configInfo.rasterizationSci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        // Take only a single sample
        configInfo.multisampleSci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        configInfo.multisampleSci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        configInfo.colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                              VK_COLOR_COMPONENT_G_BIT |
                                                              VK_COLOR_COMPONENT_B_BIT |
                                                              VK_COLOR_COMPONENT_A_BIT;

        configInfo.colorBlendSci.attachmentCount = 1;
        configInfo.colorBlendSci.pAttachments = &configInfo.colorBlendAttachmentState;
        configInfo.colorBlendSci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        // Enable depth testing
        configInfo.depthStenciSci.depthTestEnable = VK_TRUE;
        configInfo.depthStenciSci.depthWriteEnable = VK_TRUE;
        configInfo.depthStenciSci.depthCompareOp = VK_COMPARE_OP_LESS;
        configInfo.depthStenciSci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        return configInfo;
    }
}