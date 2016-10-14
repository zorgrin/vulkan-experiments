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

#include "SurfaceBackend.h"
#include "ImageObject.h"

#include <wrapper/Instance.h>
#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/Queue.h>
#include <wrapper/Surface.h>
#include <wrapper/Swapchain.h>
#include <wrapper/Fence.h>
#include <wrapper/Semaphore.h>
#include <wrapper/CommandPool.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/Image.h>
#include <wrapper/exceptions.hpp>

#include "hardcode.h"

#include <wrapper/DebugReport.h> //FIXME validation bug

namespace engine
{
namespace render
{
namespace vk
{

namespace hardcode //fixme move
{
    constexpr std::uint64_t gTimeout(1000000000);
}

struct SurfaceBackend::Private
{
    Device* device;
    QWindow* window;

    core::ReferenceObjectList<Surface>::ObjectRef surface;

    core::ReferenceObjectList<Swapchain>::ObjectRef swapchain;
    core::ReferenceObjectList<CommandPool>::ObjectRef commandPool;

    core::ReferenceObjectList<Fence>::ObjectRef fence;

    CombinedImageObject* sourceAttachment;

    struct ChainData
    {
        Private* parent;
        unsigned index;

        std::shared_ptr<ChainImageObject> image;

        VkPipelineStageFlags setupStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkSubmitInfo setupStageSubmitInfo = {};

        VkPipelineStageFlags swapStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkSubmitInfo swapStageSubmitInfo = {};

        VkPresentInfoKHR presentInfo = {};
        VkImageResolve resolveRegion = {};

        VkResult presentResult = VK_SUCCESS;

//        core::ReferenceObjectList<Semaphore>::ObjectRef setupStageSemaphore; TODO unused
        core::ReferenceObjectList<Semaphore>::ObjectRef swapStageSemaphore;
//        core::ReferenceObjectList<Semaphore>::ObjectRef presentStageSemaphore;

        core::ReferenceObjectList<CommandBuffer>::ObjectRef setupStageCommand;
        core::ReferenceObjectList<CommandBuffer>::ObjectRef swapStageCommand;

        VkExtent2D size;

        ChainData(Private* parent, unsigned index): parent(parent), index(index)
        {
            size = parent->swapchain->creationInfo().imageExtent;
            image.reset(new ChainImageObject(parent->swapchain.ptr()));
            image->reset(parent->swapchain->images().at(index));

//            setupStageSemaphore = parent->device->createSemaphore();
            swapStageSemaphore = parent->device->createSemaphore();
//            presentStageSemaphore = parent->device->createSemaphore();

            setupStageCommand = parent->commandPool->createCommandBuffer();
            {
                auto command([image = image->image()](CommandBuffer* buffer)
                {
                    {
                        VkImageMemoryBarrier info{};

                        info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        info.pNext = nullptr;
                        info.srcAccessMask = 0;
                        info.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                        info.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        info.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        info.image = image->handle();

                        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        info.subresourceRange.baseMipLevel = 0;
                        info.subresourceRange.levelCount = 1;

                        info.subresourceRange.baseArrayLayer = 0;
                        info.subresourceRange.layerCount = 1;

                        vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &info);
                    }
                });
                setupStageCommand->enqueueCommand(command);
                setupStageCommand->finalize();
            }

            {
                resolveRegion.extent.width = size.width;
                resolveRegion.extent.height = size.height;
                resolveRegion.extent.depth = 1;
                resolveRegion.srcSubresource.aspectMask =  VK_IMAGE_ASPECT_COLOR_BIT;
                resolveRegion.srcSubresource.layerCount = 1;
                resolveRegion.dstSubresource.aspectMask =  VK_IMAGE_ASPECT_COLOR_BIT;
                resolveRegion.dstSubresource.layerCount = 1;
            }

            swapStageCommand = parent->commandPool->createCommandBuffer();
        }

        ~ChainData()
        {
            image.reset();
        }

        void connectChain(ChainData* previous)
        {
            assert(previous != this);

            {
                setupStageSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                setupStageSubmitInfo.commandBufferCount = 1;
                setupStageSubmitInfo.pCommandBuffers = setupStageCommand->handlePtr();

                setupStageSubmitInfo.waitSemaphoreCount = 0; //FIXME is it correct?
                setupStageSubmitInfo.pWaitSemaphores = nullptr;

                setupStageSubmitInfo.pWaitDstStageMask = &setupStageFlags;

//                setupStageSubmitInfo.signalSemaphoreCount = 1;
//                setupStageSubmitInfo.pSignalSemaphores = setupStageSemaphore->handlePtr();
            }


            {
                swapStageSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                swapStageSubmitInfo.commandBufferCount = 1;
                swapStageSubmitInfo.pCommandBuffers = swapStageCommand->handlePtr();

                swapStageSubmitInfo.pWaitDstStageMask = &swapStageFlags;

//                swapStageSubmitInfo.waitSemaphoreCount = 1;
//                swapStageSubmitInfo.pWaitSemaphores = setupStageSemaphore->handlePtr();

                swapStageSubmitInfo.signalSemaphoreCount = 1;
                swapStageSubmitInfo.pSignalSemaphores = swapStageSemaphore->handlePtr();
            }

            {
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = swapStageSemaphore->handlePtr();

                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = parent->swapchain->handlePtr();
                presentInfo.pImageIndices = &this->index;

                presentInfo.pResults = &presentResult;
            }
        }

        void setSourceAttachment(CombinedImageObject* source)
        {
            swapStageCommand->reset();
            auto command([image = image->image(), finalAttachment = source, region = &resolveRegion, size = size](CommandBuffer* buffer)
            {
                {
                    VkImageMemoryBarrier info{};

                    info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    info.pNext = nullptr;
                    info.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; //TODO check
                    info.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                    info.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    info.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    info.image = image->handle();

                    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    info.subresourceRange.baseMipLevel = 0;
                    info.subresourceRange.levelCount = 1;

                    info.subresourceRange.baseArrayLayer = 0;
                    info.subresourceRange.layerCount = 1;

                    vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &info); //TODO check
                }

                if (finalAttachment->isMipped())
                {
                    vkCmdResolveImage(
                        buffer->handle(),
                        image->handle(),
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        finalAttachment->image()->handle(),
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        region);

                }
                else
                {
                    VkImageCopy region;
                    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.srcSubresource.mipLevel = 0;
                    region.srcSubresource.baseArrayLayer = 0;
                    region.srcSubresource.layerCount = 1;
                    region.srcOffset.x = 0;
                    region.srcOffset.y = 0;
                    region.srcOffset.z = 0;
                    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.dstSubresource.mipLevel = 0;
                    region.dstSubresource.baseArrayLayer = 0;
                    region.dstSubresource.layerCount = 1;
                    region.dstOffset.x = 0;
                    region.dstOffset.y = 0;
                    region.dstOffset.z = 0;
                    region.extent.width = size.width;
                    region.extent.height = size.height;
                    region.extent.depth = 1;

                    vkCmdCopyImage(
                        buffer->handle(),
                        finalAttachment->image()->handle(),
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image->handle(),
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &region);

                    {
                        VkImageMemoryBarrier info{};

                        info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        info.pNext = nullptr;
                        info.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
                        info.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                        info.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        info.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                        info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        info.image = image->handle();

                        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        info.subresourceRange.baseMipLevel = 0;
                        info.subresourceRange.levelCount = 1;

                        info.subresourceRange.baseArrayLayer = 0;
                        info.subresourceRange.layerCount = 1;

                        vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &info); //TODO check
                    }
                }
            });
            swapStageCommand->enqueueCommand(command);
            swapStageCommand->finalize();
        }
    };

    std::vector <std::shared_ptr<ChainData>> chain;

    std::uint32_t currentImageIndex = 0;

    Private(Device* device, QWindow* window):
        device(device), window(window), sourceAttachment(nullptr)
    {

    }
};

SurfaceBackend::SurfaceBackend(Device* device, QWindow* window):
    m_d(new Private(device, window))
{
    m_d->commandPool = device->createCommandPool();

    m_d->fence = device->createFence();
}

SurfaceBackend::~SurfaceBackend()
{
    m_d->chain.clear();

    m_d->swapchain.reset();

    m_d->surface.reset();

    m_d->commandPool.reset();
}

void SurfaceBackend::updateGeometry()
{
    m_d->chain.clear();
    m_d->swapchain.reset();

    m_d->surface.reset();

    m_d->currentImageIndex = 0;

    m_d->surface = m_d->device->physicalDevice()->instance()->createSurface(m_d->window, m_d->device->physicalDevice());

    m_d->swapchain = m_d->device->createSwapchain(m_d->surface.ptr(), hardcode::gSwapchainMinSize);

    for (unsigned i = 0; i < m_d->swapchain->size(); ++i)
    {
        std::shared_ptr<Private::ChainData> data(new Private::ChainData(m_d.get(), i));
        data->setSourceAttachment(m_d->sourceAttachment);
        m_d->chain.push_back(data);
    }
    {
        const std::size_t size(m_d->chain.size());
        assert(size > 1);
        for (unsigned i = 0; i < size; ++i)
        {
            const std::size_t previous(i == 0 ? (size - 1) : (i - 1));

            m_d->chain.at(i)->connectChain(m_d->chain.at(previous).get());
        }

        std::vector<VkSubmitInfo> submits;
        for (auto i: m_d->chain)
        {
            submits.push_back(i->setupStageSubmitInfo);
        }

        vkQueueSubmit(m_d->device->defaultQueue().handle(), static_cast<std::uint32_t>(submits.size()), submits.data(), m_d->fence->handle());
        if (!m_d->fence->wait(hardcode::gFenceWait))
        {
            throw Error(VK_TIMEOUT);
        }
        m_d->fence->reset();
    }
}

unsigned SurfaceBackend::swap()
{
    auto r(m_d->swapchain->acquireNextImage(hardcode::gTimeout, VK_NULL_HANDLE, m_d->fence->handle(), &m_d->currentImageIndex));

    if (r != VK_SUCCESS)
    {
        throw Error(r);
    }

    if (!m_d->fence->wait(hardcode::gFenceWait))
    {
        throw Error(VK_TIMEOUT);
    }

#if !defined(Q_CC_MSVC)
#   warning avoid validation layers bug for 1.0.26
#else
#   pragma message("avoid validation layers bug for 1.0.26")
#endif
    m_d->device->physicalDevice()->instance()->debugReporter()->setIsEnabled(false);
    m_d->fence->reset();
    m_d->device->physicalDevice()->instance()->debugReporter()->setIsEnabled(true);

    assert(m_d->currentImageIndex < m_d->chain.size());

    m_d->device->defaultQueue().submit(m_d->chain.at(m_d->currentImageIndex)->swapStageSubmitInfo, m_d->fence->handle());
    if (!m_d->fence->wait(hardcode::gFenceWait))
    {
        throw Error(VK_TIMEOUT);
    }
    m_d->fence->reset();


    m_d->device->present(m_d->device->defaultQueue().handle(), m_d->chain.at(m_d->currentImageIndex)->presentInfo);
    m_d->device->defaultQueue().wait(); //FIXME
    if (m_d->chain.at(m_d->currentImageIndex)->presentResult != VK_SUCCESS)
    {
        throw Error(m_d->chain.at(m_d->currentImageIndex)->presentResult);
    }

    return m_d->currentImageIndex;
}

void SurfaceBackend::setSourceAttachment(CombinedImageObject* source)
{
    assert(source);

    m_d->sourceAttachment = source;

    for (auto &i: m_d->chain)
    {
        i->setSourceAttachment(source);
    }
}


} // namespace vk
} // namespace render
} // namespace engine
