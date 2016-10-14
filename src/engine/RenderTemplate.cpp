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

#include "RenderTemplate.h"

#include <wrapper/Device.h>
#include <wrapper/RenderPass.h>
#include <wrapper/reference-list.hpp>

namespace engine
{
namespace render
{
namespace vk
{

struct RenderTemplate::Private
{
    Device* device;
    std::vector<VkAttachmentDescription> attachments;

    std::list<core::ReferenceObjectList<RenderPass>::ObjectRef> renderPasses;

    Private(Device* device, const std::vector<VkAttachmentDescription>& attachments): device(device), attachments(attachments)
    {

    }
};

RenderTemplate::RenderTemplate(Device* device, const std::vector<VkAttachmentDescription>& attachments):
    m_d(new Private(device, attachments))
{
}

RenderTemplate::~RenderTemplate()
{
    m_d->renderPasses.clear();
}

RenderPass* RenderTemplate::createRenderPass(const std::vector<VkSubpassDescription>& subpasses, const std::vector<VkSubpassDependency>& dependencyGraph)
{
    auto result(m_d->device->createRenderPass(m_d->attachments, subpasses, dependencyGraph));
    m_d->renderPasses.push_back(result);

    return result.ptr();
}

void RenderTemplate::destroyRenderPass(RenderPass* renderPass)
{
    m_d->renderPasses.remove_if([&renderPass](auto ref)->bool
    {
        return ref.ptr() == renderPass;
    });
}

} // namespace vk
} // namespace render
} // namespace engine
