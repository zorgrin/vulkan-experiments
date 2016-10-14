// BSD 3-Clause License
//
// Copyright (c) 2016, Michael Vasilyev
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "TestRenderScene.h"
#include "Test.h"

#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/RenderPass.h>
#include <wrapper/ShaderModule.h>
#include <wrapper/PipelineLayout.h>
#include <wrapper/PipelineCache.h>
#include <wrapper/Pipeline.h>
#include <wrapper/DescriptorSetLayout.h>
#include <wrapper/Image.h>

#include <engine/RenderTemplate.h>
#include <engine/ImageObject.h>
#include <engine/RenderBatch.h>
#include <engine/RenderObject.h>
#include <engine/SimpleBufferObject.h>
#include <engine/RenderObjectGroup.h>
#include <engine/Utils.h>
#include <engine/Camera.h>
#include <engine/CameraController.h>
#include <engine/hardcode.h>


#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

namespace engine
{
namespace render
{
namespace vk
{
namespace test
{

struct TestRenderScene::Private
{
    Device* device;
    QObject* eventSource;

    std::unique_ptr<ICamera> camera;
    std::unique_ptr<CameraController> cameraController;

    std::unique_ptr<RenderTemplate> renderTemplate;
    RenderPass* drawPass = nullptr;
    std::unique_ptr<CombinedImageObject> depthStencilImage;
    std::vector<std::shared_ptr<CombinedImageObject>> colorAttachments;

    core::ReferenceObjectList<PipelineCache>::ObjectRef pipelineCache;

    std::unique_ptr<RenderBatch> renderBatch;

    std::map<std::string, std::map<RenderBatch::ShaderStage, core::ReferenceObjectList<ShaderModule>::ObjectRef>> shaders;

    VkExtent2D size = {};

    struct MeshPass
    {
        std::unique_ptr<RenderObjectGroup> group;

        core::ReferenceObjectList<Pipeline>::ObjectRef pipeline;

        std::unique_ptr<test::Test> mesh;
    };
    std::unique_ptr<MeshPass> testMesh;

    struct TestQuad
    {
        std::unique_ptr<RenderObjectGroup> group;

        core::ReferenceObjectList<Pipeline>::ObjectRef pipeline;

        std::unique_ptr<SimpleBufferObject> position;
        std::unique_ptr<SimpleBufferObject> index;

        std::unique_ptr<RenderObject> object;
    };

    std::unique_ptr<TestQuad> testQuad;

    Private(Device* device, QObject* eventSource): device(device), eventSource(eventSource)
    {

    }

    void createMeshPipeline()
    {
        constexpr unsigned subpass(0);
        const PipelineLayout* layout(testMesh->group->pipelineLayout());
        std::vector<VkVertexInputBindingDescription> inputBinding; //FIXME hardcoded
        { //position
            VkVertexInputBindingDescription desc{};
            desc.binding = 0;
            desc.stride = 3 * sizeof(float);
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            inputBinding.push_back(desc);
        }
        { //texCoord
            VkVertexInputBindingDescription desc{};
            desc.binding = 1;
            desc.stride = 2 * sizeof(float);
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            inputBinding.push_back(desc);
        }
        std::vector<VkVertexInputAttributeDescription> inputAttributes;
        {
            VkVertexInputAttributeDescription desc{};
            desc.location = 0;
            desc.binding = 0;
            desc.format = VK_FORMAT_R32G32B32_SFLOAT; //float3
            desc.offset = 0;

            inputAttributes.push_back(desc);
        }
        {
            VkVertexInputAttributeDescription desc{};
            desc.location = 1;
            desc.binding = 1;
            desc.format = VK_FORMAT_R32G32_SFLOAT; //float2
            desc.offset = 0;

            inputAttributes.push_back(desc);
        }

        VkPipelineVertexInputStateCreateInfo inputState{};
        {
            inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            inputState.pNext = nullptr;
            inputState.flags = 0;
            inputState.vertexBindingDescriptionCount = static_cast<std::uint32_t>(inputBinding.size());
            inputState.pVertexBindingDescriptions = inputBinding.data();
            inputState.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(inputAttributes.size());
            inputState.pVertexAttributeDescriptions = inputAttributes.data();
        }

        assert(shaders.size() == 2);
        std::vector<VkPipelineShaderStageCreateInfo> stages(2);
        {
            stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].pNext = nullptr;
            stages[0].flags = 0;
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module = shaders.at("gpass").at(RenderBatch::ShaderStage::Vertex)->handle();
            stages[0].pName = "main"; //FIXME hardcoded
            stages[0].pSpecializationInfo = nullptr;
        }
        {
            stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[1].pNext = nullptr;
            stages[1].flags = 0;
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module = shaders.at("gpass").at(RenderBatch::ShaderStage::Fragment)->handle();
            stages[1].pName = "main"; //FIXME hardcoded
            stages[1].pSpecializationInfo = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo assemblyState{};
        {
            assemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            assemblyState.pNext = nullptr;
            assemblyState.flags = 0;
            assemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            assemblyState.primitiveRestartEnable = VK_FALSE;
        }

        VkPipelineTessellationStateCreateInfo tesselationState{}; //TODO tes/tsc
        {
            tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tesselationState.pNext = nullptr;
            tesselationState.flags = 0;
            tesselationState.patchControlPoints = 0;
        }

        VkViewport dummyViewport{};
        VkRect2D dummyScissor{};

        VkPipelineViewportStateCreateInfo viewportState{};
        {
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.pNext = nullptr;
            viewportState.flags = 0;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &dummyViewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &dummyScissor;
        }

        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        {
    //        SOFT_NOT_IMPLEMENTED();
            rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationState.pNext = nullptr;
            rasterizationState.depthClampEnable = VK_FALSE;
            rasterizationState.rasterizerDiscardEnable = VK_FALSE;
            rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationState.cullMode = VK_CULL_MODE_NONE;
            rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE; //TODO VK_FRONT_FACE_COUNTER_CLOCKWISE
            rasterizationState.depthBiasEnable = VK_FALSE;
            rasterizationState.depthBiasConstantFactor = 0;
            rasterizationState.depthBiasClamp = 0;
            rasterizationState.depthBiasSlopeFactor = 0;
            rasterizationState.lineWidth = 1.0;
        }

        VkPipelineMultisampleStateCreateInfo multisampleState{};
        {
            multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.pNext = nullptr;
            multisampleState.flags = 0;
            multisampleState.rasterizationSamples = hardcode::gSamples;
            multisampleState.sampleShadingEnable = VK_FALSE;
            multisampleState.minSampleShading = 0;
            multisampleState.pSampleMask = nullptr;
            multisampleState.alphaToCoverageEnable = VK_FALSE;
            multisampleState.alphaToOneEnable = VK_FALSE;
        }
        VkPipelineDepthStencilStateCreateInfo depthStencilState{};
        {
            depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencilState.pNext = nullptr;
            depthStencilState.flags = 0;
            depthStencilState.depthTestEnable = VK_TRUE;
            depthStencilState.depthWriteEnable = VK_TRUE;
            depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencilState.depthBoundsTestEnable = VK_FALSE;
            depthStencilState.stencilTestEnable = VK_FALSE; //TODO enable stencil test
            {
                depthStencilState.front.failOp = VK_STENCIL_OP_KEEP;
                depthStencilState.front.passOp = VK_STENCIL_OP_KEEP;
                depthStencilState.front.depthFailOp = VK_STENCIL_OP_KEEP;
                depthStencilState.front.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                depthStencilState.front.compareMask = 0;
                depthStencilState.front.writeMask = 0;
                depthStencilState.front.reference = 0;
            }
            depthStencilState.back = depthStencilState.front;
            depthStencilState.minDepthBounds = 0;
            depthStencilState.maxDepthBounds = 0; //TODO dynamic?
        }

        std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState;
        for (unsigned i = 0; i < colorAttachments.size(); ++i)
        {
            VkPipelineColorBlendAttachmentState state{};
            state.blendEnable = VK_FALSE;
            state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.colorBlendOp = VK_BLEND_OP_ADD;
            state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.alphaBlendOp = VK_BLEND_OP_ADD;
            state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            blendAttachmentState.push_back(state);
        }
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        {
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.pNext = nullptr;
            colorBlendState.flags = 0;
            colorBlendState.logicOpEnable = VK_FALSE; //TODO
            colorBlendState.logicOp = VK_LOGIC_OP_COPY; //VK_LOGIC_OP_CLEAR
            colorBlendState.attachmentCount = static_cast<std::uint32_t>(blendAttachmentState.size());
            colorBlendState.pAttachments = blendAttachmentState.data();
    //        colorBlendState.blendConstants = { 0, 0, 0, 0 };

        }

        std::vector<VkDynamicState> dynamicStates;
        {
            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
        }
        VkPipelineDynamicStateCreateInfo dynamicState{};
        {
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pNext = nullptr;
            dynamicState.flags = 0;
            dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();
        }

        VkGraphicsPipelineCreateInfo info{};
        {
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0; //TODO VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT
            info.stageCount = static_cast<std::uint32_t>(stages.size());
            info.pStages = stages.data();
            info.pVertexInputState = &inputState;
            info.pInputAssemblyState = &assemblyState;
            info.pTessellationState = &tesselationState;
            info.pViewportState = &viewportState;
            info.pRasterizationState = &rasterizationState;
            info.pMultisampleState = &multisampleState;
            info.pDepthStencilState = &depthStencilState;
            info.pColorBlendState = &colorBlendState;
            info.pDynamicState = &dynamicState;
            info.layout = layout->handle();
            info.renderPass = drawPass->handle();
            info.subpass = subpass;

            info.basePipelineHandle = VK_NULL_HANDLE;
            info.basePipelineIndex = -1;
        }
        testMesh->pipeline = pipelineCache->createPipeline(info);
    }

    void createQuadPipeline() //TODO pipeline builder
    {
        constexpr unsigned subpass(1);
        const PipelineLayout* layout(testQuad->group->pipelineLayout());
        std::vector<VkVertexInputBindingDescription> inputBinding; //FIXME hardcoded
        { //position
            VkVertexInputBindingDescription desc{};
            desc.binding = 0;
            desc.stride = 3 * sizeof(float);
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            inputBinding.push_back(desc);
        }

        std::vector<VkVertexInputAttributeDescription> inputAttributes;
        {
            VkVertexInputAttributeDescription desc{};
            desc.location = 0;
            desc.binding = 0;
            desc.format = VK_FORMAT_R32G32B32_SFLOAT; //float3
            desc.offset = 0;

            inputAttributes.push_back(desc);
        }

        VkPipelineVertexInputStateCreateInfo inputState{};
        {
            inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            inputState.pNext = nullptr;
            inputState.flags = 0;
            inputState.vertexBindingDescriptionCount = static_cast<std::uint32_t>(inputBinding.size());
            inputState.pVertexBindingDescriptions = inputBinding.data();
            inputState.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(inputAttributes.size());
            inputState.pVertexAttributeDescriptions = inputAttributes.data();
        }

        assert(shaders.size() == 2);
        std::vector<VkPipelineShaderStageCreateInfo> stages(2);
        {
            stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].pNext = nullptr;
            stages[0].flags = 0;
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module = shaders.at("test").at(RenderBatch::ShaderStage::Vertex)->handle();
            stages[0].pName = "main"; //FIXME hardcoded
            stages[0].pSpecializationInfo = nullptr;
        }
        {
            stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[1].pNext = nullptr;
            stages[1].flags = 0;
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module = shaders.at("test").at(RenderBatch::ShaderStage::Fragment)->handle();
            stages[1].pName = "main"; //FIXME hardcoded
            stages[1].pSpecializationInfo = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo assemblyState{};
        {
            assemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            assemblyState.pNext = nullptr;
            assemblyState.flags = 0;
            assemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            assemblyState.primitiveRestartEnable = VK_FALSE;
        }

        VkPipelineTessellationStateCreateInfo tesselationState{}; //TODO tes/tsc
        {
            tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tesselationState.pNext = nullptr;
            tesselationState.flags = 0;
            tesselationState.patchControlPoints = 0;
        }

        VkViewport dummyViewport{};
        VkRect2D dummyScissor{};

        VkPipelineViewportStateCreateInfo viewportState{};
        {
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.pNext = nullptr;
            viewportState.flags = 0;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &dummyViewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &dummyScissor;
        }

        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        {
    //        SOFT_NOT_IMPLEMENTED();
            rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationState.pNext = nullptr;
            rasterizationState.depthClampEnable = VK_FALSE;
            rasterizationState.rasterizerDiscardEnable = VK_FALSE;
            rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizationState.cullMode = VK_CULL_MODE_NONE;
            rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE; //TODO VK_FRONT_FACE_COUNTER_CLOCKWISE
            rasterizationState.depthBiasEnable = VK_FALSE;
            rasterizationState.depthBiasConstantFactor = 0;
            rasterizationState.depthBiasClamp = 0;
            rasterizationState.depthBiasSlopeFactor = 0;
            rasterizationState.lineWidth = 1.0;
        }

        VkPipelineMultisampleStateCreateInfo multisampleState{};
        {
            multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.pNext = nullptr;
            multisampleState.flags = 0;
            multisampleState.rasterizationSamples = hardcode::gSamples;
            multisampleState.sampleShadingEnable = VK_FALSE;
            multisampleState.minSampleShading = 0;
            multisampleState.pSampleMask = nullptr;
            multisampleState.alphaToCoverageEnable = VK_FALSE;
            multisampleState.alphaToOneEnable = VK_FALSE;
        }

        std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState;
        for (unsigned i = 0; i < colorAttachments.size(); ++i)
        {
            VkPipelineColorBlendAttachmentState state{};
            state.blendEnable = VK_FALSE;
            state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.colorBlendOp = VK_BLEND_OP_ADD;
            state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            state.alphaBlendOp = VK_BLEND_OP_ADD;
            state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            blendAttachmentState.push_back(state);
        }
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        {
            colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlendState.pNext = nullptr;
            colorBlendState.flags = 0;
            colorBlendState.logicOpEnable = VK_FALSE; //TODO
            colorBlendState.logicOp = VK_LOGIC_OP_COPY; //VK_LOGIC_OP_CLEAR
            colorBlendState.attachmentCount = static_cast<std::uint32_t>(blendAttachmentState.size());
            colorBlendState.pAttachments = blendAttachmentState.data();
    //        colorBlendState.blendConstants = { 0, 0, 0, 0 };

        }

        std::vector<VkDynamicState> dynamicStates;
        {
            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
        }
        VkPipelineDynamicStateCreateInfo dynamicState{};
        {
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pNext = nullptr;
            dynamicState.flags = 0;
            dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();
        }

        VkGraphicsPipelineCreateInfo info{};
        {
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0; //TODO VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT
            info.stageCount = static_cast<std::uint32_t>(stages.size());
            info.pStages = stages.data();
            info.pVertexInputState = &inputState;
            info.pInputAssemblyState = &assemblyState;
            info.pTessellationState = &tesselationState;
            info.pViewportState = &viewportState;
            info.pRasterizationState = &rasterizationState;
            info.pMultisampleState = &multisampleState;
            info.pDepthStencilState = nullptr;
            info.pColorBlendState = &colorBlendState;
            info.pDynamicState = &dynamicState;
            info.layout = layout->handle();
            info.renderPass = drawPass->handle();
            info.subpass = subpass;

            info.basePipelineHandle = VK_NULL_HANDLE;
            info.basePipelineIndex = -1;
        }
        testQuad->pipeline = pipelineCache->createPipeline(info);
    }
};

TestRenderScene::TestRenderScene(Device* device, QObject* eventSource): m_d(new Private(device, eventSource))
{
    m_d->eventSource->installEventFilter(this);
    {
        QDir binDir(QCoreApplication::applicationDirPath());

        std::map <std::string, std::map<RenderBatch::ShaderStage, std::string>> stages;
        for (auto i: binDir.entryInfoList(QStringList() << "*.spirv", QDir::Files))
        {
            auto shader(i.baseName());

            //FIXME quick and dirty
            if (i.fileName().endsWith(".vs.spirv"))
            {
                stages[shader.toStdString()][RenderBatch::ShaderStage::Vertex] = i.absoluteFilePath().toStdString();
            }
            else if (i.fileName().endsWith(".fs.spirv"))
            {
                stages[shader.toStdString()][RenderBatch::ShaderStage::Fragment] = i.absoluteFilePath().toStdString();
            }
            else
            {
                NOT_IMPLEMENTED();
            }
        }
        for (auto technique: stages)
        {
            for (auto stage: technique.second)
            {
                auto module(m_d->device->createShaderModule(stage.second));
                m_d->shaders[technique.first][stage.first] = module;
            }
            qDebug() << "shaded" << technique.first.c_str() << "is loaded";

        }
    }

    m_d->depthStencilImage.reset(new CombinedImageObject(m_d->device));
    m_d->depthStencilImage->reset(
        VK_IMAGE_TYPE_2D,
        m_d->device->physicalDevice()->defaultDepthStencilFormat(),
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        hardcode::gSamples,
        hardcode::gSamples != VK_SAMPLE_COUNT_1_BIT
    );

    //TODO remove unused usages
    {
        std::shared_ptr<CombinedImageObject> diffuse(new CombinedImageObject(m_d->device));
        diffuse->reset(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            hardcode::gSamples,
            hardcode::gSamples != VK_SAMPLE_COUNT_1_BIT
        );

        m_d->colorAttachments.push_back(diffuse);
    }

    {
        std::shared_ptr<CombinedImageObject> normal(new CombinedImageObject(m_d->device));
        normal->reset(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R16G16B16A16_SFLOAT, //TODO RGB16
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            hardcode::gSamples,
            hardcode::gSamples != VK_SAMPLE_COUNT_1_BIT
        );
        m_d->colorAttachments.push_back(normal);
    }
    {
        std::shared_ptr<CombinedImageObject> fragCoord(new CombinedImageObject(m_d->device));
        fragCoord->reset(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            hardcode::gSamples,
            hardcode::gSamples != VK_SAMPLE_COUNT_1_BIT
        );
        m_d->colorAttachments.push_back(fragCoord);
    }

    {
        std::shared_ptr<CombinedImageObject> meshInstancePrimitive(new CombinedImageObject(m_d->device));
        meshInstancePrimitive->reset(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SINT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            hardcode::gSamples,
            hardcode::gSamples != VK_SAMPLE_COUNT_1_BIT
        );
        m_d->colorAttachments.push_back(meshInstancePrimitive);
    }


    {
        std::vector<VkAttachmentDescription> attachments;
        {
            VkAttachmentDescription attachment{};
            attachment.flags = 0;
            attachment.format = m_d->depthStencilImage->format();
            attachment.samples = hardcode::gSamples;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachments.push_back(attachment);

            for (unsigned i = 0; i < m_d->colorAttachments.size(); ++i)
            {
                VkAttachmentDescription attachment{};
                attachment.flags = 0;
                attachment.format = m_d->colorAttachments.at(i)->format();
                attachment.samples = hardcode::gSamples;
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                attachments.push_back(attachment);
            }
        }
        m_d->renderTemplate.reset(new RenderTemplate(m_d->device, attachments));
    }


    {
        std::vector<VkSubpassDescription> subpasses;

        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference{};

        constexpr unsigned colorOffset(1);

        {
            depthReference.attachment = 0;
            depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        for (unsigned i = 0; i < m_d->colorAttachments.size(); ++i)
        {
            VkAttachmentReference reference{};
            reference.attachment = i + colorOffset;
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorReferences.push_back(reference);
        }

        { //subpass 0
            VkSubpassDescription subpass{};
            subpass.flags = 0;
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = 0;
            subpass.pInputAttachments = nullptr;
            subpass.colorAttachmentCount = static_cast<std::uint32_t>(colorReferences.size());
            subpass.pColorAttachments = colorReferences.data();
            subpass.pResolveAttachments = nullptr;
            subpass.pDepthStencilAttachment = &depthReference;
            subpass.preserveAttachmentCount = 0;
            subpass.pPreserveAttachments = nullptr;

            subpasses.push_back(subpass);
        }

        std::vector<VkAttachmentReference> subpass1Input;
        {
            VkAttachmentReference reference{};
            reference.attachment = 0 + colorOffset; //diffuse
            reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            subpass1Input.push_back(reference);
        }

        std::vector<std::uint32_t> preserved; //FIXME validator test
        preserved.push_back(2); //normal
        preserved.push_back(3); //fragCoord
        preserved.push_back(4); //IDs

        //FIXME validator issue #127 "Attachment <#> not written by FS"

        { //subpass 1
            VkSubpassDescription subpass{};
            subpass.flags = 0;
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = static_cast<std::uint32_t>(subpass1Input.size());
            subpass.pInputAttachments = subpass1Input.data();
            subpass.colorAttachmentCount = static_cast<std::uint32_t>(colorReferences.size());
            subpass.pColorAttachments = colorReferences.data();
            subpass.pResolveAttachments = nullptr;
            subpass.pDepthStencilAttachment = nullptr;
            subpass.preserveAttachmentCount = static_cast<std::uint32_t>(preserved.size());
            subpass.pPreserveAttachments = preserved.data();

            subpasses.push_back(subpass);
        }

        std::vector<VkSubpassDependency> graph;
        {
            VkSubpassDependency pass1Dependency{};
            pass1Dependency.srcSubpass = 0;
            pass1Dependency.dstSubpass = 1;
            pass1Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            pass1Dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            pass1Dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            pass1Dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            pass1Dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            graph.push_back(pass1Dependency);
        }
//        {
//            VkSubpassDependency selfDependency{};
//            selfDependency.srcSubpass = 1;
//            selfDependency.dstSubpass = 1;
//            selfDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//            selfDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//            selfDependency.srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
//            selfDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//            selfDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//            graph.push_back(selfDependency);
//        }

        m_d->drawPass = m_d->renderTemplate->createRenderPass(subpasses, graph);

        RenderBatch::AttachmentList drawAttachments;
        drawAttachments.push_back(m_d->depthStencilImage.get());

        for (auto i: m_d->colorAttachments)
            drawAttachments.push_back(i.get());


        std::vector<RenderBatch::SubpassInputBinding> subpassBindings;
        {
            {
                RenderBatch::SubpassInputBinding binding;
                subpassBindings.push_back(binding); // empty
            }
            {
                RenderBatch::SubpassInputBinding binding;
                {
                    RenderBatch::SubpassInputBinding::Attachment attachment;
                    attachment.layout.binding = 0;
                    attachment.layout.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    attachment.layout.descriptorCount = 1;
                    attachment.layout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    attachment.layout.pImmutableSamplers = nullptr;

                    attachment.reference = colorOffset;

                    binding.attachments.push_back(attachment);
                }
                subpassBindings.push_back(binding);
            }
        }

        m_d->renderBatch.reset(new RenderBatch(m_d->device, m_d->drawPass, drawAttachments, subpassBindings));
    }

    m_d->pipelineCache = m_d->device->createPipelineCache();

    {
        RenderObjectGroup::DescriptorSetFormat groupSetFormat;
        {
            groupSetFormat.uniforms.insert(0); // camera data
        }
        RenderObjectGroup::DescriptorSetFormat objectSetFormat;
        {
            objectSetFormat.storages.insert(0); // instance matrices array
            objectSetFormat.combinedImages.insert(1); // diffuse texture
        }

        std::vector<std::map<RenderBatch::ShaderStage, ShaderModule*>> stages;//FIXME
        {
            std::map<RenderBatch::ShaderStage, ShaderModule*> s;
            auto first(m_d->shaders.at("gpass"));
            for (auto i: first)
            {
                s[i.first] = i.second.ptr();
            }
            stages.push_back(s);

            s = {};
            first = m_d->shaders.at("test");
            for (auto i: first)
            {
                s[i.first] = i.second.ptr();
            }
            stages.push_back(s);

        }

        {
            constexpr unsigned geometrySubpass(0); //TODO hardcode
            m_d->testMesh.reset(new Private::MeshPass());
            m_d->testMesh->group.reset(new RenderObjectGroup(m_d->device, m_d->renderBatch->attachmentLayout(geometrySubpass), groupSetFormat, objectSetFormat));
            {
                auto cameraBuffer(m_d->testMesh->group->uniform(0));
                cameraBuffer->reset(Camera::dataSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            }
            m_d->testMesh->group->finalize();


            m_d->createMeshPipeline();

            m_d->testMesh->mesh.reset(new test::Test(m_d->device, this));
        }

        {
            RenderObjectGroup::DescriptorSetFormat groupSetFormat; // dummy
            RenderObjectGroup::DescriptorSetFormat objectSetFormat; // dummy

            m_d->testQuad.reset(new Private::TestQuad());
            m_d->testQuad->group.reset(new RenderObjectGroup(m_d->device, m_d->renderBatch->attachmentLayout(1), groupSetFormat, objectSetFormat));
            m_d->testQuad->group->finalize();

            m_d->createQuadPipeline();
        }

    }


    m_d->renderBatch->addGroup(0, m_d->testMesh->group.get(), m_d->testMesh->pipeline.ptr());

    m_d->renderBatch->addGroup(1, m_d->testQuad->group.get(), m_d->testQuad->pipeline.ptr());

    {
        RenderObject::InputData inputData;
        {
            using Data = std::vector<QVector3D>;
            Data data;

            data.push_back(QVector3D(-1.0f, -1.0f,  0.25f));
            data.push_back(QVector3D(1.0f, -1.0f,  0.25f));
            data.push_back(QVector3D(1.0f,  1.0f,  0.25f));

            const std::size_t size(data.size() * sizeof(Data::value_type));
            m_d->testQuad->position.reset(new SimpleBufferObject(device));
            m_d->testQuad->position->reset(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            m_d->testQuad->position->write(0, data.data(), size);

            inputData.attributes[0] = m_d->testQuad->position.get();
        }
        {
            using Data = std::vector<unsigned>;
            Data data;

            data.push_back(0);
            data.push_back(1);
            data.push_back(2);

            const std::size_t size(data.size() * sizeof(Data::value_type));
            m_d->testQuad->index.reset(new SimpleBufferObject(device));
            m_d->testQuad->index->reset(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            m_d->testQuad->index->write(0, data.data(), size);

            inputData.indexBuffer = m_d->testQuad->index.get();
            inputData.indexCount = static_cast<unsigned>(data.size());
        }

        m_d->testQuad->object.reset(new RenderObject(device, m_d->testQuad->group->objectLayout(), inputData));
        m_d->testQuad->object->setInstanceCount(1);

        m_d->testQuad->group->addObject(m_d->testQuad->object.get());
    }

    {
        auto cameraBuffer(m_d->testMesh->group->uniform(0));
        m_d->camera.reset(new Camera(cameraBuffer));
        QMatrix4x4 projection;
        projection.perspective(45, 1.0f, 0.1f, std::numeric_limits<float>::max());
        m_d->camera->setProjection(projection);
        m_d->camera->setEye(QVector3D(0.0, 0.0, -300.0));

        m_d->cameraController.reset(new CameraController(m_d->camera.get()));
    }

}

TestRenderScene::~TestRenderScene()
{
    m_d->eventSource->removeEventFilter(this);
    m_d->cameraController.reset();
    m_d->camera.reset();

    m_d->renderBatch.reset();

    m_d->testMesh.reset();
    m_d->testQuad.reset();

    m_d->renderTemplate->destroyRenderPass(m_d->drawPass);
    m_d->renderTemplate.reset();
}


void TestRenderScene::test_addMeshPassObject(RenderObject* object)
{
    m_d->testMesh->group->addObject(object);

    m_d->renderBatch->invalidate(); //FIXME optimize
}

void TestRenderScene::render(float elapsed)
{
    step(elapsed);
    m_d->renderBatch->render();
}

void TestRenderScene::setup(CommandBuffer* buffer)
{
    setImageLayout(m_d->depthStencilImage->image(), buffer, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    for (auto i: m_d->colorAttachments)
    {
        setImageLayout(i->image(), buffer, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    }
}

CombinedImageObject* TestRenderScene::finalAttachment() const
{
    return m_d->colorAttachments.front().get();
}

RenderObjectGroup* TestRenderScene::test_meshGroup() const
{
    return m_d->testMesh->group.get();
}

void TestRenderScene::resize(unsigned width, unsigned height)
{
    m_d->size = VkExtent2D{width, height};

    VkExtent3D size{width, height, 1};
    m_d->depthStencilImage->resize(size);

    for (auto i: m_d->colorAttachments)
        i->resize(size);

    m_d->depthStencilImage->resetView();

    for (auto i: m_d->colorAttachments)
        i->resetView();

    m_d->renderBatch->resize(size.width, size.height);

    {
        if (height > 0)
        {
            QMatrix4x4 projection;//FIXME controller->resizeEvent
            projection.perspective(45, static_cast<float>(width) / static_cast<float>(height), 0.1f, std::numeric_limits<float>::max());
            m_d->cameraController->camera()->setProjection(projection);
        }
    }
}

bool TestRenderScene::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyRelease)
    {
        m_d->cameraController->keyReleaseEvent(static_cast<QKeyEvent*>(event));

        return false;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        m_d->cameraController->keyPressEvent(static_cast<QKeyEvent*>(event));

        return false;
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        m_d->cameraController->mousePressEvent(static_cast<QMouseEvent*>(event));

        return false;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        m_d->cameraController->mouseReleaseEvent(static_cast<QMouseEvent*>(event));

        return false;
    }
    else if (event->type() == QEvent::MouseMove)
    {
        m_d->cameraController->mouseMoveEvent(static_cast<QMouseEvent*>(event));

        return false;
    }
    else if (event->type() == QEvent::Wheel)
    {
        m_d->cameraController->wheelEvent(static_cast<QWheelEvent*>(event));

        return false;
    }

    return false;
}

void TestRenderScene::step(float elapsed)
{
    m_d->cameraController->step(elapsed);
}

void TestRenderScene::keyPressEvent(QKeyEvent *event)
{
    m_d->cameraController->keyPressEvent(event);
}

void TestRenderScene::keyReleaseEvent(QKeyEvent *event)
{
    m_d->cameraController->keyReleaseEvent(event);
}

void TestRenderScene::mousePressEvent(QMouseEvent *event)
{
    m_d->cameraController->mousePressEvent(event);
}

void TestRenderScene::mouseMoveEvent(QMouseEvent *event)
{
    m_d->cameraController->mouseMoveEvent(event);
}

void TestRenderScene::mouseReleaseEvent(QMouseEvent *event)
{
    m_d->cameraController->mouseReleaseEvent(event);
}

void TestRenderScene::wheelEvent(QWheelEvent *event)
{
    m_d->cameraController->wheelEvent(event);
}


} // namespace test
} // namespace vk
} // namespace render
} // namespace engine
