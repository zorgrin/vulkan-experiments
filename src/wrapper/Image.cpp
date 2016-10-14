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


#include "Image.h"

#include "Allocator.h"
#include "Device.h"
#include "DeviceMemory.h"
#include "exceptions.hpp"
#include "ImageView.h"

#include <cmath>
#include "PhysicalDevice.h"

namespace engine
{
namespace render
{
namespace vk
{

namespace //TODO move to util
{
    static unsigned maxMipsFor(const VkExtent3D& size)
    {
        return static_cast<unsigned>(std::floor(std::log(std::max(std::max(size.width, size.height), size.depth)) / std::log(2.0))) + 1;
    }
}

struct ImageBase::Private
{
    Device* device;
    
    core::ReferenceObjectList<ImageView> views;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(Device* device): device(device), views(friendDeleter<ImageView>)
    {
    }
};

ImageBase::ImageBase(Device* device): 
    Handle(VK_NULL_HANDLE), m_d(new Private(device))
{

}

ImageBase::~ImageBase()
{
    assert(m_d->views.isEmpty());
}

Device* ImageBase::device() const
{
    return m_d->device;
}

core::ReferenceObjectList<ImageView>::ObjectRef ImageBase::createView(const VkImageViewCreateInfo &info)
{
    assert(info.image == handle());
    auto copy(info);
    copy.image = handle();

    return m_d->views.append(new ImageView(this, copy));
}

void ImageBase::clearViews()
{
    m_d->views.clear();
}

////////////////////////////////////////////////////////////////////////////////
SwapchainImage::SwapchainImage(Device* device, const VkImage& handle): ImageBase(device)
{
    m_handle = handle;
}

SwapchainImage::~SwapchainImage()
{
    ImageBase::clearViews();
}

////////////////////////////////////////////////////////////////////////////////
Image::Image(Device* device, const VkImageCreateInfo& info): ImageBase(device), CreationInfo(info)
{
    validateStruct(m_info);

    auto r(vkCreateImage(device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

Image::~Image()
{
    ImageBase::clearViews();

    if (m_handle != VK_NULL_HANDLE)
        vkDestroyImage(device()->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Image* Image::create(
    Device* device,
    const VkImageType &type,
    const VkFormat &format,
    const VkExtent3D &size,
    const VkImageLayout& layout,
    const VkImageTiling& tiling,
    const VkImageUsageFlags& usage,
    const VkSampleCountFlagBits& samples,
    bool mipped)
{
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.imageType = type;
    info.format = format;
    info.extent = size;
    info.mipLevels = mipped ? maxMipsFor(size) : 1;
    info.arrayLayers = 1; //TODO array
    info.samples = samples;
    info.tiling = tiling;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = nullptr;
    info.initialLayout = layout;

//    if (mipped)
//    {
//        VkImageFormatProperties props{};
//        auto r = vkGetPhysicalDeviceImageFormatProperties(device->physicalDevice()->handle(), format, type, tiling, usage, info.flags, &props);
//        qDebug() << r;
//        qDebug() << size.width << size.height << "mipped" << maxMipsFor(size) << props.maxMipLevels << props.maxResourceSize << props.maxArrayLayers;
//        r = vkGetPhysicalDeviceImageFormatProperties(device->physicalDevice()->handle(), format, VK_IMAGE_TYPE_3D, tiling, usage, info.flags, &props);
//        qDebug() << r;
//        qDebug() << size.width << size.height << "mipped" << maxMipsFor(size) << props.maxMipLevels << props.maxResourceSize << props.maxArrayLayers;
//    }

    return new Image(device, info);
}

void Image::bind(DeviceMemory* memory, std::size_t memoryOffset)
{
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(ImageBase::device()->handle(), m_handle, &requirements);
    assert((memoryOffset % requirements.alignment) == 0);

    auto r(vkBindImageMemory(device()->handle(), m_handle, memory->handle(), memoryOffset));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

} // namespace vk
} // namespace render
} // namespace engine
