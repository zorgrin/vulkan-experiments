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

#include "RenderBatch.h"
#include "RenderTemplate.h"
#include "RenderObjectGroup.h"
#include "ImageObject.h"
#include "RenderObject.h"
#include "hardcode.h"

#include <wrapper/Device.h>
#include <wrapper/Queue.h>
#include <wrapper/RenderPass.h>
#include <wrapper/CommandPool.h>
#include <wrapper/Framebuffer.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/DescriptorPool.h>
#include <wrapper/DescriptorSet.h>
#include <wrapper/ImageView.h>
#include <wrapper/PipelineCache.h>
#include <wrapper/PipelineLayout.h>
#include <wrapper/Pipeline.h>
#include <wrapper/Fence.h>
#include <wrapper/Image.h>
#include <wrapper/Sampler.h>
#include <wrapper/exceptions.hpp>

#include <QtGui/QColor>

namespace engine
{
namespace render
{
namespace vk
{

namespace hardcode //FIXME move
{
    const QColor gClearColor(Qt::darkRed);

    constexpr float gDefaultDepth(1.0);
    constexpr unsigned gDefaultStencil(0);
}

namespace //FIXME move
{
    template <typename T>
    VkClearColorValue toClearColor(const QColor& c);

    template <>
    VkClearColorValue toClearColor<float>(const QColor& c)
    {
        VkClearColorValue result;
        result.float32[0] = static_cast<float>(c.redF());
        result.float32[1] = static_cast<float>(c.greenF());
        result.float32[2] = static_cast<float>(c.blueF());
        result.float32[3] = static_cast<float>(c.alphaF());

        return result;
    }
}

struct RenderBatch::Private
{
    Device* device;
    RenderPass* renderPass;
    AttachmentList attachments;

    struct SubpassData
    {
        core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef attachmentsLayout;
        struct ObjectGroup
        {
            RenderObjectGroup* group;
            Pipeline* pipeline;
        };
        std::list<ObjectGroup> objectGroups;

        std::map<SubpassInputBinding::BindingPoint, SubpassInputBinding::AttachmentReference> inputAttachments;
    };
    std::vector<std::shared_ptr<SubpassData>> subpasses;

    core::ReferenceObjectList<DescriptorPool>::ObjectRef descriptorPool;

    struct FramebufferObject
    {
        Private* parent;

        core::ReferenceObjectList<Framebuffer>::ObjectRef framebuffer;

        std::vector<core::ReferenceObjectList<DescriptorSet>::ObjectRef> subpassSets;

        VkSubmitInfo submitDraw;
        VkSubmitInfo submitClear;

        VkPipelineStageFlags clearFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // referred from submitClear
        VkPipelineStageFlags drawFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // referred from submitDraw

        core::ReferenceObjectList<CommandPool>::ObjectRef commandPool;
        core::ReferenceObjectList<CommandBuffer>::ObjectRef clearBuffer;
        std::list<core::ReferenceObjectList<CommandBuffer>::ObjectRef> drawBuffers;
        std::vector<VkCommandBuffer> cachedBuffers;

        VkViewport viewport = {};
        VkRect2D scissor = {};

        bool finalized = false;
        VkExtent2D size;

        core::ReferenceObjectList<Fence>::ObjectRef fence;

        std::vector<VkClearValue> cachedClearValues;

        FramebufferObject(Private* parent):
            parent(parent), submitDraw{}
        {
            viewport.maxDepth = 1.0;

            assert(parent->attachments.size() > 1);

            fence = parent->device->createFence();

            commandPool = parent->device->createCommandPool();

            cachedClearValues.resize(parent->attachments.size());
            {
                cachedClearValues[0].depthStencil.depth = hardcode::gDefaultDepth;
                cachedClearValues[0].depthStencil.stencil = hardcode::gDefaultStencil;

                cachedClearValues[1].color = toClearColor<float>(hardcode::gClearColor);
            }

            clearBuffer = commandPool->createCommandBuffer();

            {
                submitDraw.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitDraw.pNext = nullptr;

                submitDraw.waitSemaphoreCount = 0; //TODO semaphore
                submitDraw.pWaitSemaphores = nullptr;

                submitDraw.pWaitDstStageMask = &drawFlags;

                submitDraw.commandBufferCount = 0;
                submitDraw.pCommandBuffers = nullptr;

                submitDraw.signalSemaphoreCount = 0; //TODO semaphore
                submitDraw.pSignalSemaphores = nullptr;
            }

            {
                submitClear.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitClear.pNext = nullptr;

                submitClear.waitSemaphoreCount = 0; //TODO semaphore
                submitClear.pWaitSemaphores = nullptr;

                submitClear.pWaitDstStageMask = &clearFlags;

                submitClear.commandBufferCount = 1;
                submitClear.pCommandBuffers = clearBuffer->handlePtr();

                submitClear.signalSemaphoreCount = 0; //TODO semaphore
                submitClear.pSignalSemaphores = nullptr;
            }

            for (auto i: parent->subpasses)
            {
                auto subpassSet(parent->descriptorPool->createDescriptorSet(i->attachmentsLayout.ptr()));
                subpassSets.push_back(subpassSet);
            }
        }

        ~FramebufferObject()
        {
            drawBuffers.clear();
            clearBuffer.reset();
            fence.reset();
            framebuffer.reset();
            subpassSets.clear();
            commandPool.reset();
        }

        void submit()
        {
            assert(submitDraw.commandBufferCount > 0);

            parent->device->defaultQueue().submit(submitClear, fence->handle());
            bool ok(fence->wait(hardcode::gFenceWait));
            assert(ok);
            fence->reset();

            parent->device->defaultQueue().submit(submitDraw, fence->handle());
            ok = fence->wait(hardcode::gFenceWait);
            assert(ok);
            fence->reset();
        }

        void reset()
        {
            cachedBuffers.clear();
            drawBuffers.clear();
            framebuffer.reset();
        }

        void resize(unsigned width, unsigned height)
        {
            scissor.extent.width = width;
            viewport.width = static_cast<float>(width);
            scissor.extent.height = height;
            viewport.height = static_cast<float>(height);

            std::vector<ImageView*> views;
            for (auto i: parent->attachments)
            {
                views.push_back(i->planarImageView());
            }
            framebuffer = parent->device->createFramebuffer(parent->renderPass, views, size, 1);

            for (unsigned i = 0; i < parent->subpasses.size(); ++i)
            {
                Private::SubpassData* subpass(parent->subpasses.at(i).get());
                auto ds(subpassSets.at(i));
                std::vector<VkDescriptorImageInfo> descriptors;
                std::vector<VkWriteDescriptorSet> writes;

                for (auto j: subpass->inputAttachments)
                {
                    auto bindingPoint(j.first);
                    auto attachmentIndex(j.second);
                    {
                        VkDescriptorImageInfo info{};
                        info.sampler = VK_NULL_HANDLE;
                        assert(attachmentIndex < parent->attachments.size());
                        auto image(parent->attachments.at(attachmentIndex));
                        info.imageView = image->imageView()->handle();
                        info.imageLayout = image->imageLayout();

                        descriptors.push_back(info);
                    }

                    {
                        VkWriteDescriptorSet descriptorInfo{};
                        descriptorInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        descriptorInfo.pNext = nullptr;
                        descriptorInfo.dstSet = ds->handle();
                        descriptorInfo.dstBinding = bindingPoint;
                        descriptorInfo.dstArrayElement = 0;
                        descriptorInfo.descriptorCount = 1;
                        descriptorInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        descriptorInfo.pImageInfo = &descriptors.back();
                        descriptorInfo.pBufferInfo = nullptr;
                        descriptorInfo.pTexelBufferView = nullptr;

                        validateStruct(descriptorInfo);
                        writes.push_back(descriptorInfo);
                    }
                }
                assert(i != 1 || !writes.empty()); //FIXME subpass1
                if (!writes.empty())
                    vkUpdateDescriptorSets(parent->device->handle(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
            }

        }

        void rebuildBuffer(CommandBuffer* buffer, unsigned subpass, RenderObjectGroup* group, Pipeline* pipeline, RenderObject* object)
        {
            buffer->enqueueCommand([&](CommandBuffer* buffer)
            {
                auto buildDraw([&](CommandBuffer* buffer)->void
                {
                    vkCmdSetViewport(buffer->handle(), 0, 1, &viewport);
                    vkCmdSetScissor(buffer->handle(), 0, 1, &scissor);

                    const unsigned subpassCount(parent->renderPass->creationInfo().subpassCount); //FIXME RenderPass::subpassCount()
                    for (unsigned currentSubpass = 0; currentSubpass < subpassCount; ++currentSubpass)
                    {
                        if (subpass == currentSubpass)
                        {
                            SOFT_NOT_IMPLEMENTED();
                            //try to insert barrier write->read->write

                            assert(pipeline->type() == VK_PIPELINE_BIND_POINT_GRAPHICS);
                            vkCmdBindPipeline(buffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle()); //FIXME subpass specific

                            std::vector<VkDescriptorSet> sets;
                            VkDescriptorSet inputAttachmentSet(subpassSets.at(currentSubpass)->handle());
                            sets.push_back(inputAttachmentSet);
                            {
                                auto groupSet(group->groupSet());
                                if (groupSet != nullptr)
                                    sets.push_back(groupSet->handle());

                                auto objectSet(object->descriptorSet());
                                if (objectSet != nullptr)
                                    sets.push_back(objectSet->handle());
                            }
                            assert(group->pipelineLayout()->creationInfo().setLayoutCount == sets.size());

                            vkCmdBindDescriptorSets(buffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, group->pipelineLayout()->handle(), 0, static_cast<std::uint32_t>(sets.size()), sets.data(), 0, nullptr); //FIXME hardcode

                            object->buildSubpass(buffer);
                        }

                        if (currentSubpass < (subpassCount - 1))
                        {
                            vkCmdNextSubpass(buffer->handle(), VK_SUBPASS_CONTENTS_INLINE);
                        }
                    }
                });

                buffer->appendRenderPass(buildDraw, VK_SUBPASS_CONTENTS_INLINE, parent->renderPass->handle(), framebuffer->handle(), VkOffset2D{}, size, std::vector<VkClearValue>());
            });
            buffer->finalize();
        }

        void finalize()
        {
            cachedBuffers.clear();
            drawBuffers.clear();

            for (unsigned subpassIndex = 0; subpassIndex < parent->subpasses.size(); ++subpassIndex)
            {
                auto subpass(parent->subpasses.at(subpassIndex));
                for (auto group: subpass->objectGroups)
                {
                    auto objects(group.group->objects());
                    if (!objects.empty())
                    {

                        for (auto object: objects)
                        {
                            core::ReferenceObjectList<CommandBuffer>::ObjectRef buffer(commandPool->createCommandBuffer());
                            rebuildBuffer(buffer.ptr(), subpassIndex, group.group, group.pipeline, object);

                            drawBuffers.push_back(buffer);
                            cachedBuffers.push_back(buffer->handle());
                        }
                    }
                }

                {
                    submitDraw.commandBufferCount = static_cast<std::uint32_t>(cachedBuffers.size());
                    submitDraw.pCommandBuffers = cachedBuffers.data();
                }
                { // clear phase
                    auto buffer(clearBuffer.ptr());
                    buffer->reset();
                    buffer->enqueueCommand([&](CommandBuffer* buffer)
                    {
                        //TODO transition is required?
//                        {
//                            std::vector<VkImageMemoryBarrier> barriers;
//
//                            VkImageMemoryBarrier barrier;
//                            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//                            barrier.pNext = nullptr;
//                            barrier.srcAccessMask = 0;
//                            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//                            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//                            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//
//                            barrier.subresourceRange.baseMipLevel = 0;
//                            barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
//
//                            barrier.subresourceRange.baseArrayLayer = 0;
//                            barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
//
//                            { // depth/stencil
//                                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//                                barrier.image = parent->attachments.at(0)->image()->handle();
//                                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//                                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
//
//                                barriers.push_back(barrier);
//                            }
//
//                            // color
//                            for (unsigned i = 1; i < parent->attachments.size(); ++i)
//                            {
//                                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//                                barrier.image = parent->attachments.at(i)->image()->handle();
//                                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//                                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//
//                                barriers.push_back(barrier);
//                            }
//
//                            vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
//                        }

                        //FIXME fix for multisampled
                        VkImageSubresourceRange range{};
                        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                        range.baseMipLevel = 0;
                        range.levelCount = VK_REMAINING_MIP_LEVELS;
                        range.baseArrayLayer = 0;
                        range.layerCount = VK_REMAINING_ARRAY_LAYERS;

                        auto clearDS(cachedClearValues.at(0).depthStencil);
                        vkCmdClearDepthStencilImage(buffer->handle(), parent->attachments.at(0)->image()->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDS, 1, &range);

                        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        for (unsigned i = 1; i < parent->attachments.size(); ++i)
                        {
                            auto clearColor(cachedClearValues.at(i).color);
                            vkCmdClearColorImage(buffer->handle(), parent->attachments.at(i)->image()->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
                        }
                    });
                    buffer->finalize();
                }


            }

            finalized = true;

#if 0 //TODO remove debug
            {
                debugPass.submit = {};
                {
                    debugPass.submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    debugPass.submit.pNext = nullptr;

                    debugPass.submit.waitSemaphoreCount = 0; //TODO setupSemaphore
                    debugPass.submit.pWaitSemaphores = nullptr;

                    debugPass.submit.pWaitDstStageMask = &drawFlags;

                    debugPass.submit.commandBufferCount = 0;
                    debugPass.submit.pCommandBuffers = nullptr;

                    debugPass.submit.signalSemaphoreCount = 0; //FIXME presentSemaphore
                    debugPass.submit.pSignalSemaphores = nullptr;
                }

                debugPass.pool = device->createCommandPool();
                debugPass.cmd = debugPass.pool->createGroup(1);
                auto buffer(debugPass.cmd->buffer(0));
                buffer.enqueueCommand([&](CommandBuffer* buffer){
                    auto dummy([&](CommandBuffer* buffer){

                    });
                    buffer->appendRenderPass(dummy, VK_SUBPASS_CONTENTS_INLINE, renderPass->handle(), framebuffer0->handle(), VkOffset2D{}, size, clearValues);
                });
                buffer.finalize();

                debugPass.submit.commandBufferCount = 1;
                debugPass.submit.pCommandBuffers = debugPass.cmd->handlesPtr();
            }
#endif
        }

    };
    std::unique_ptr<FramebufferObject> fbo;

    Private(Device* device, RenderPass* renderPass, const AttachmentList& attachments):
        device(device), renderPass(renderPass), attachments(attachments)
    {

    }

};

RenderBatch::RenderBatch(
    Device* device,
    RenderPass* renderPass,
    const AttachmentList& attachments,
    const std::vector<SubpassInputBinding>& subpasses):
        m_d(new Private(device, renderPass, attachments))
{

    std::map<VkDescriptorType, unsigned> typeSlots;

    for (unsigned i = 0; i < subpasses.size(); ++i)
    {
        std::shared_ptr<Private::SubpassData> subpass(new Private::SubpassData);

        std::vector<VkDescriptorSetLayoutBinding> layout;
        for (auto i: subpasses.at(i).attachments)
        {
            layout.push_back(i.layout);
            subpass->inputAttachments[i.layout.binding] = i.reference;
        }

        subpass->attachmentsLayout = m_d->device->createDescriptorSetLayout(layout);
        typeSlots[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT] += static_cast<unsigned>(subpasses.at(i).attachments.size());

        m_d->subpasses.push_back(subpass);
    }

    m_d->descriptorPool = device->createDescriptorPool(static_cast<unsigned>(subpasses.size()), typeSlots);

    m_d->fbo.reset(new Private::FramebufferObject(m_d.get()));
}

RenderBatch::~RenderBatch()
{

    m_d->fbo.reset();
    m_d->subpasses.clear();
    m_d->descriptorPool.reset();
}

void RenderBatch::addGroup(SubpassIndex subpass, RenderObjectGroup* group, Pipeline* pipeline)
{
    assert(subpass < m_d->subpasses.size());
    Private::SubpassData::ObjectGroup objectGroup;
    objectGroup.group = group;
    objectGroup.pipeline = pipeline;
    m_d->subpasses.at(subpass)->objectGroups.push_back(objectGroup);
    qDebug() << "group added" << group << subpass;

    invalidate();
}

void RenderBatch::render()
{
    if (!m_d->fbo->finalized)
        m_d->fbo->finalize();

    m_d->fbo->submit();
}

void RenderBatch::reset()
{
    m_d->fbo->reset();
}

void RenderBatch::resize(unsigned width, unsigned height)
{
    m_d->fbo->size = VkExtent2D{width, height};

    m_d->fbo->resize(width, height);

    invalidate();
}

RenderPass* RenderBatch::renderPass() const
{
    return m_d->renderPass;
}

void RenderBatch::invalidate()
{
    m_d->fbo->finalized = false;
}

DescriptorSetLayout* RenderBatch::attachmentLayout(SubpassIndex subpass) const
{
    assert(subpass < m_d->subpasses.size());
    return m_d->subpasses.at(subpass)->attachmentsLayout.ptr();
}

} // namespace vk
} // namespace render
} // namespace engine
