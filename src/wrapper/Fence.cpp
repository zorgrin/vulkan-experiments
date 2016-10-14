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


#include "Fence.h"

#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct Fence::Private
{
    Device* device;
    
    Private(Device* device): device(device)
    {
    }
};

Fence::Fence(Device* device, const VkFenceCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    auto r(vkCreateFence(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

Fence::~Fence()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyFence(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

bool Fence::isReady() const
{
    auto r(vkGetFenceStatus(m_d->device->handle(), m_handle));
    switch (r)
    {
    case VK_SUCCESS:
        return true;
    case VK_NOT_READY:
        return false;
    default:
        throw Error(r);
    }
}

void Fence::reset()
{
    auto r(vkResetFences(m_d->device->handle(), 1, &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

bool Fence::wait(std::uint64_t nsecs)
{
    auto r(vkWaitForFences(m_d->device->handle(), 1, &m_handle, VK_TRUE, nsecs));
    switch (r)
    {
    case VK_SUCCESS:
        return true;
    case VK_TIMEOUT:
        return false;
    default:
        throw Error(r);
    }
}

Fence* Fence::create(Device* device, bool signaled)
{
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    return new Fence(device, info);
}


} // namespace vk
} // namespace render
} // namespace engine
