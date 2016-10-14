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


#include "RenderPass.h"

#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

namespace
{
    static void validateReferences(const VkRenderPassCreateInfo& info)
    {
        for (unsigned sp = 0; sp < info.subpassCount; ++sp)
        {
            for (unsigned ref = 0; ref < info.pSubpasses[sp].inputAttachmentCount; ++ref)
                assert(info.pSubpasses[sp].pInputAttachments[ref].attachment < info.attachmentCount);
            for (unsigned ref = 0; ref < info.pSubpasses[sp].colorAttachmentCount; ++ref)
            {
                assert(info.pSubpasses[sp].pColorAttachments[ref].attachment < info.attachmentCount);
                if (info.pSubpasses[sp].pResolveAttachments != nullptr)
                    assert((info.pSubpasses[sp].pResolveAttachments[ref].attachment < info.attachmentCount) || (info.pSubpasses[sp].pResolveAttachments[ref].attachment == VK_ATTACHMENT_UNUSED));
            }
            for (unsigned ref = 0; ref < info.pSubpasses[sp].preserveAttachmentCount; ++ref)
                assert(info.pSubpasses[sp].pPreserveAttachments[ref] < info.attachmentCount);
        }
    }

}

struct RenderPass::Private
{
    Device* device;
    
    Private(Device* device): device(device)
    {
    }
};

RenderPass::RenderPass(Device* device, const VkRenderPassCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);
    validateReferences(info);

    auto r(vkCreateRenderPass(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

RenderPass::~RenderPass()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyRenderPass(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Device* RenderPass::device() const
{
    return m_d->device;
}

RenderPass* RenderPass::create(Device* device, const std::vector<VkAttachmentDescription>& attachments, const std::vector<VkSubpassDescription>& subpasses, const std::vector<VkSubpassDependency>& dependency)
{
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    info.pAttachments = attachments.data();
    info.subpassCount = static_cast<std::uint32_t>(subpasses.size());
    info.pSubpasses = subpasses.data();
    info.dependencyCount = static_cast<std::uint32_t>(dependency.size());
    info.pDependencies = dependency.data();

    return new RenderPass(device, info);
}

} // namespace vk
} // namespace render
} // namespace engine
