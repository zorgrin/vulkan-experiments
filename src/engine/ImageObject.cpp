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

#include "ImageObject.h"
#include "Utils.h"

#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/DeviceMemory.h>
#include <wrapper/Image.h>
#include <wrapper/ImageView.h>
#include <wrapper/Sampler.h>
#include <wrapper/CommandPool.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/Queue.h>

#include <wrapper/Swapchain.h>

#include <QtGui/QImage>

namespace engine
{
namespace render
{
namespace vk
{

struct ChainImageObject::Private
{
    Swapchain* swapchain;

    std::unique_ptr<SwapchainImage> image;
    core::ReferenceObjectList<ImageView>::ObjectRef view;

    Private(Swapchain* swapchain): swapchain(swapchain)
    {

    }

    void resetImageAndView()
    {
        view.reset();
        image.reset();
    }
};

ChainImageObject::ChainImageObject(Swapchain* swapchain): m_d(new Private(swapchain))
{

}

ChainImageObject::~ChainImageObject()
{
    m_d->resetImageAndView();
}

SwapchainImage* ChainImageObject::image() const
{
    return m_d->image.get();
}

ImageView* ChainImageObject::imageView() const
{
    return m_d->view.ptr();
}

void ChainImageObject::reset(const VkImage& handle)
{
    m_d->resetImageAndView();

    m_d->image.reset(new SwapchainImage(m_d->swapchain->device(), handle));

    VkImageViewCreateInfo viewInfo{};

    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.flags = 0;
    viewInfo.image = handle;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

    auto swapchainInfo(m_d->swapchain->creationInfo());
    viewInfo.format = swapchainInfo.imageFormat;

    VkComponentMapping mapping
    {
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A
    };
    viewInfo.components = mapping;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    m_d->view = m_d->image->createView(viewInfo);
}

////////////////////////////////////////////////////////////////////////////////

CombinedImageObject::CombinedImageObject(Device* device):
    _device(device),
    _currentSize{},
    _isValid(false)
{

}

CombinedImageObject::~CombinedImageObject()
{
    _sampler.reset();
    _view.reset();
    _planarView.reset();
    _image.reset();
    _memory.reset();
}

void CombinedImageObject::reset(
    const VkImageType& type,
    const VkFormat& format,
    const VkImageAspectFlags& aspectMask,
    const VkImageUsageFlags& usage,
    const VkImageViewType& viewType,
    const VkImageLayout& layout,
    const VkMemoryPropertyFlags& memoryFlags,
    const VkImageTiling& tiling,
    const VkSampleCountFlagBits& samples,
    bool mipped)
{
    _sampler.reset();
    _view.reset();
    _planarView.reset();
    _image.reset();
    _memory.reset();

    _type = type;
    _format = format;
    _aspectMask = aspectMask;
    _usage = usage;
    _viewType = viewType;
    _layout = layout;
    _memoryFlags = memoryFlags;
    _tiling = tiling;
    _samples = samples;
    _mipped = mipped;

    _isValid = true;
//    resize(_currentSize);
}

Image* CombinedImageObject::image() const
{
    return _image.ptr();
}

ImageView* CombinedImageObject::imageView() const
{
    assert(_image.ptr());
    return _view.ptr();
}

ImageView* CombinedImageObject::planarImageView() const
{
    assert(_image.ptr());
    return _planarView.ptr();
}

Sampler* CombinedImageObject::sampler() const
{
    return _sampler.ptr();
}

void CombinedImageObject::resize(const VkExtent3D& size)
{
    assert(_isValid);
    _currentSize = size;
    if (!_image.isNull())
    {
        assert(!_view.isNull());
        _view.reset();
        _planarView.reset();

        _image.reset();
    }
    _image = _device->createImage(_type, _format, size, _layout, _tiling, _usage, _samples, _mipped);

    VkMemoryRequirements requirements(DeviceMemory::requirements(_device, _image->handle()));
    _memory = _device->allocate(requirements.size, _device->physicalDevice()->memoryFor(requirements, _memoryFlags));

    _image->bind(_memory.ptr(), 0);

    _sampler = _device->createSampler();
    //TODO cache
}

void CombinedImageObject::resetView()
{
    assert(_isValid);
    VkImageViewCreateInfo viewInfo{};
    {
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = 0;
        viewInfo.image = _image->handle();
        viewInfo.viewType = _viewType;

        viewInfo.format = _format;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        viewInfo.subresourceRange.aspectMask = _aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }

    _view = _image->createView(viewInfo);

    viewInfo.subresourceRange.levelCount = 1;
    _planarView = _image->createView(viewInfo);
}

DeviceMemory* CombinedImageObject::memory() const
{
    return _memory.ptr();
}

VkImageType CombinedImageObject::type() const
{
    return _type;
}

VkFormat CombinedImageObject::format() const
{
    return _format;
}

VkImageAspectFlags CombinedImageObject::aspectMask() const
{
    return _aspectMask;
}

VkImageLayout CombinedImageObject::imageLayout() const
{
    return _layout;
}

QImage CombinedImageObject::readImage() const
{
    assert(_isValid);
    //FIXME test only
    QImage result(static_cast<int>(_currentSize.width), static_cast<int>(_currentSize.height), converters::convert(_format));
    assert(_memory->creationInfo().allocationSize == VkDeviceSize(result.bytesPerLine() * result.height()));
    _memory->invalidate(0, _memory->creationInfo().allocationSize);

    _memory->map(0, _memory->creationInfo().allocationSize);
    auto ptr(_memory->data<char>());
    std::copy(ptr, ptr + _memory->creationInfo().allocationSize, result.bits());
    _memory->unmap();

    return result;
}

bool CombinedImageObject::isMipped() const
{
    return _mipped;
}

} // namespace vk
} // namespace render
} // namespace engine
