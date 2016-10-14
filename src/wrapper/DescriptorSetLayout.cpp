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


#include "DescriptorSetLayout.h"
#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct DescriptorSetLayout::Private
{
    Device* device;
    
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    Private(Device* device): device(device)
    {
    }
};

DescriptorSetLayout::DescriptorSetLayout(Device* device, const VkDescriptorSetLayoutCreateInfo &info):
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);
    std::copy(info.pBindings, info.pBindings + info.bindingCount, std::back_inserter(m_d->bindings));
    m_info.bindingCount = static_cast<std::uint32_t>(m_d->bindings.size());
    m_info.pBindings = m_d->bindings.data();

    auto r(vkCreateDescriptorSetLayout(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Device* DescriptorSetLayout::device() const
{
    return m_d->device;
}

DescriptorSetLayout* DescriptorSetLayout::create(Device* device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
    VkDescriptorSetLayoutCreateInfo info{};

    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.bindingCount = bindings.empty() ? 0 : static_cast<std::uint32_t>(bindings.size());
    info.pBindings = bindings.empty() ? nullptr : bindings.data();

    return new DescriptorSetLayout(device, info);
}

std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayout::bindings() const
{
    return m_d->bindings;
}

} // namespace vk
} // namespace render
} // namespace engine
