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


#include "DeviceMemory.h"
#include "PhysicalDevice.h"

#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct DeviceMemory::Private
{
    Device* device;
        
    bool canBeMapped;
    void* mappedData;
    
    Private(Device* device): device(device), canBeMapped(false), mappedData(nullptr)
    {
    }
};

DeviceMemory::DeviceMemory(Device* device, const VkMemoryAllocateInfo &info):
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    assert(info.memoryTypeIndex < m_d->device->physicalDevice()->memoryProperties().memoryTypeCount);
    if ((m_d->device->physicalDevice()->memoryProperties().memoryTypes[info.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        m_d->canBeMapped = true;

    auto r(vkAllocateMemory(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

DeviceMemory::~DeviceMemory()
{
    if (m_d->mappedData != nullptr)
        unmap();
    if (m_handle != VK_NULL_HANDLE)
        vkFreeMemory(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Device* DeviceMemory::device() const
{
    return m_d->device;
}

void* DeviceMemory::map(std::ptrdiff_t offset, std::size_t size)
{
    if (!m_d->canBeMapped)
        throw core::Exception("memory does not support mapping");

    if (m_d->mappedData != nullptr)
        throw core::Exception("already mapped");

    constexpr VkMemoryMapFlags flags(0);

    auto r(vkMapMemory(m_d->device->handle(), m_handle, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size), flags, &m_d->mappedData));
    CHECK_VK_RESULT(r, VK_SUCCESS);
    assert(m_d->mappedData);

    return m_d->mappedData;
}

void DeviceMemory::unmap()
{
    if (m_d->mappedData == nullptr)
        throw core::Exception("not mapped");

    vkUnmapMemory(m_d->device->handle(), m_handle);
    m_d->mappedData = nullptr;
}

void DeviceMemory::flush(std::ptrdiff_t offset, std::size_t size)
{
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = m_handle;
    range.offset = static_cast<VkDeviceSize>(offset);
    range.size = static_cast<VkDeviceSize>(size);

    auto r(vkFlushMappedMemoryRanges(m_d->device->handle(), 1, &range));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

void DeviceMemory::invalidate(std::ptrdiff_t offset, std::size_t size)
{
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.memory = m_handle;
    range.offset = static_cast<VkDeviceSize>(offset);
    range.size = static_cast<VkDeviceSize>(size);

    auto r(vkInvalidateMappedMemoryRanges(m_d->device->handle(), 1, &range));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

VkMemoryRequirements DeviceMemory::requirements(Device* device, const VkBuffer &buffer)
{
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device->handle(), buffer, &requirements);
    return requirements;
}

VkMemoryRequirements DeviceMemory::requirements(Device* device, const VkImage &image)
{
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device->handle(), image, &requirements);
    return requirements;
}

DeviceMemory* DeviceMemory::create(Device* device, std::size_t size, unsigned memoryIndex)
{
    VkMemoryAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.allocationSize = static_cast<VkDeviceSize>(size);
    info.memoryTypeIndex = memoryIndex;

    return new DeviceMemory(device, info);
}

void* DeviceMemory::mappedData() const
{
    return m_d->mappedData;
}

} // namespace vk
} // namespace render
} // namespace engine
