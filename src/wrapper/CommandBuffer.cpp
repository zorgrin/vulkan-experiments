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


#include "CommandBuffer.h"

#include "CommandPool.h"
#include "Device.h"
#include "exceptions.hpp"
#include "Image.h"

namespace engine
{
namespace render
{
namespace vk
{

struct CommandBuffer::Private
{
    CommandPool* pool;

    VkCommandBufferBeginInfo beginInfo;

    std::list<std::function<void(CommandBuffer* buffer)>> enqueuedCommands;
    bool finalized;


    Private(CommandPool* pool):
        pool(pool), beginInfo{}, finalized(false)
    {
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
    }
};

CommandBuffer::CommandBuffer(CommandPool* pool, VkCommandBufferLevel level):
    Handle(VK_NULL_HANDLE), CreationInfo(), m_d(new Private(pool))
{
    m_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    m_info.pNext = nullptr;
    m_info.commandPool = pool->handle();
    m_info.level = level;
    m_info.commandBufferCount = 1;
    validateStruct(m_info);

    auto r(vkAllocateCommandBuffers(m_d->pool->device()->handle(), &m_info, &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

CommandBuffer::~CommandBuffer()
{
    if (m_handle != VK_NULL_HANDLE)
        vkFreeCommandBuffers(m_d->pool->device()->handle(), m_d->pool->handle(), 1, &m_handle);
}

CommandPool* CommandBuffer::pool() const
{
    return m_d->pool;
}

void CommandBuffer::appendRenderPass(
    std::function<void(CommandBuffer* buffer)> callback,
    const VkSubpassContents& contents,
    const VkRenderPass& renderPass,
    const VkFramebuffer& framebuffer,
    const VkOffset2D &offset,
    const VkExtent2D &extent,
    const std::vector<VkClearValue>& clearValues)
{
    assert(renderPass != VK_NULL_HANDLE);
    assert(framebuffer != VK_NULL_HANDLE);
    VkRenderPassBeginInfo info{};
    {
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext = nullptr;
        info.renderPass = renderPass;
        info.framebuffer = framebuffer;
        info.renderArea.offset = offset;
        info.renderArea.extent = extent;
        info.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
        info.pClearValues = clearValues.empty() ? nullptr : clearValues.data();
    }

    vkCmdBeginRenderPass(m_handle, &info, contents);
    {
        callback(this);
    }
    vkCmdEndRenderPass(m_handle);
}

void CommandBuffer::enqueueCommand(std::function<void(CommandBuffer* buffer)> callback)
{
    m_d->enqueuedCommands.push_back(callback);
}

void CommandBuffer::finalize()
{
    if (m_d->finalized)
        throw core::Exception("already finalized");
    if (m_d->enqueuedCommands.empty())
        throw core::Exception("no commands to enqueue");

    auto r(vkBeginCommandBuffer(handle(), &m_d->beginInfo));
    CHECK_VK_RESULT(r, VK_SUCCESS);

    while (!m_d->enqueuedCommands.empty())
    {
        auto callback(m_d->enqueuedCommands.front());
        m_d->enqueuedCommands.pop_front();

        try
        {
            callback(this);
        }
        catch (core::Exception& e) //TODO specific exception
        {
            qCritical() << e.what();
            r = vkEndCommandBuffer(m_handle);
            CHECK_VK_RESULT(r, VK_SUCCESS);

            reset();
            throw;
        }
    }

    r = vkEndCommandBuffer(m_handle);
    CHECK_VK_RESULT(r, VK_SUCCESS);
    m_d->finalized = true;
}

void CommandBuffer::reset()
{
    m_d->finalized = false;
    m_d->enqueuedCommands.clear();
    auto r(vkResetCommandBuffer(m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

} // namespace vk
} // namespace render
} // namespace engine
