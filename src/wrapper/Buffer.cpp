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


#include "Buffer.h"

#include "Allocator.h"
#include "Device.h"
#include "DeviceMemory.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct Buffer::Private
{
    Device* device;
    
    Private(Device* device): device(device)
    {
    }
};

Buffer::Buffer(Device* device, const VkBufferCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);


    auto r(vkCreateBuffer(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

Buffer::~Buffer()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyBuffer(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

void Buffer::bind(DeviceMemory* memory, std::size_t memoryOffset)
{
    {//sanity check
        VkMemoryRequirements requirements(DeviceMemory::requirements(memory->device(), m_handle));
        assert((memoryOffset % requirements.alignment) == 0);
    }


    auto r(vkBindBufferMemory(memory->device()->handle(), m_handle, memory->handle(), memoryOffset));
    CHECK_VK_RESULT(r, VK_SUCCESS);


}

Buffer* Buffer::create(Device* device, std::size_t size, VkBufferUsageFlagBits usage)
{

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0; //TODO flags
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//TODO mode
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = nullptr;

    return new Buffer(device, info);
}

std::size_t Buffer::size() const
{
    return m_info.size;
}

} // namespace vk
} // namespace render
} // namespace engine
