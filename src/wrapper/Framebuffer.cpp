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


#include "Framebuffer.h"

#include "Allocator.h"
#include "Device.h"
#include "RenderPass.h"
#include "ImageView.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct Framebuffer::Private
{
    Device* device;
    
    Private(Device* device): device(device)
    {
    }
};

Framebuffer::Framebuffer(Device* device, const VkFramebufferCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    auto r(vkCreateFramebuffer(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

Framebuffer::~Framebuffer()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyFramebuffer(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Framebuffer* Framebuffer::create(RenderPass* renderPass, const std::vector<ImageView*> &attachments, const VkExtent2D &size, unsigned layers)
{
    assert(layers > 0);
    assert(size.width > 0 && size.height > 0);
    assert(renderPass->creationInfo().attachmentCount == attachments.size());

    std::vector<VkImageView> attachmentHandles(attachments.size());
    for (unsigned i = 0; i < attachments.size(); ++i)
        attachmentHandles[i] = attachments.at(i)->handle();

    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.renderPass = renderPass->handle();
    info.attachmentCount = static_cast<std::uint32_t>(attachmentHandles.size());
    info.pAttachments = attachmentHandles.data();
    info.width = size.width;
    info.height = size.height;
    info.layers = layers;

    return new Framebuffer(renderPass->device(), info);
}

Device* Framebuffer::device() const
{
    return m_d->device;
}

} // namespace vk
} // namespace render
} // namespace engine
