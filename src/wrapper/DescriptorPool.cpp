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


#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "Device.h"
#include "Allocator.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct DescriptorPool::Private
{
    Device* device;
        
    core::ReferenceObjectList<DescriptorSet> sets;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(Device* device): device(device), sets(friendDeleter<DescriptorSet>)
    {
    }
};

DescriptorPool::DescriptorPool(Device* device, const VkDescriptorPoolCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    auto r(vkCreateDescriptorPool(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

DescriptorPool::~DescriptorPool()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

DescriptorPool* DescriptorPool::create(Device* device, unsigned maxSets, const std::map<VkDescriptorType, unsigned>& typeSlots)
{
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = maxSets;

    std::vector<VkDescriptorPoolSize> poolSizes;
    for (auto i: typeSlots)
    {
        const VkDescriptorType type(i.first);
        const std::uint32_t typeCapacity(i.second);
        if (i.second > 0)
            poolSizes.push_back(VkDescriptorPoolSize{ type, typeCapacity });
    }
    assert(!poolSizes.empty());
    info.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    info.pPoolSizes = poolSizes.data();

    return new DescriptorPool(device, info);
}

Device* DescriptorPool::device() const
{
    return m_d->device;
}

core::ReferenceObjectList<DescriptorSet>::ObjectRef DescriptorPool::createDescriptorSet(const DescriptorSetLayout* layout)
{
    return m_d->sets.append(new DescriptorSet(this, layout));
}

} // namespace vk
} // namespace render
} // namespace engine
