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


#pragma once

#include "Handle.hpp"

#include <functional>

namespace engine
{
namespace render
{
namespace vk
{

class CommandPool;
class Image;

class CommandBuffer Q_DECL_FINAL: public Handle<VkCommandBuffer>, public CreationInfo<VkCommandBufferAllocateInfo>
{
    Q_DISABLE_COPY(CommandBuffer)

private:
    explicit CommandBuffer(CommandPool* pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    ~CommandBuffer();

    friend class CommandPool;

public:
    CommandPool* pool() const;

    void setViewport(const VkViewport &value);
    void setScissors(const std::vector<VkRect2D> &value);
    void bindDescriptorSets(const std::vector<VkDescriptorSet> &value);

    void appendRenderPass(
        std::function<void(CommandBuffer* buffer)> callback,
        const VkSubpassContents& contents,
        const VkRenderPass& renderPass,
        const VkFramebuffer& framebuffer,
        const VkOffset2D &offset,
        const VkExtent2D &extent,
        const std::vector<VkClearValue>& clearValues);
    void enqueueCommand(std::function<void(CommandBuffer* buffer)> callback);
    void finalize();
    void reset();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

} // namespace vk
} // namespace render
} // namespace engine
