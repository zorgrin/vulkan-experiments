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

#pragma once

#include <wrapper/defines.h>
#include <wrapper/reference-list.hpp>

namespace engine
{
namespace render
{
namespace vk
{

class Device;
class Image;
class SwapchainImage;
class ImageView;
class DeviceMemory;
class Sampler;

class Swapchain;
//TODO ImageObject base


class ChainImageObject Q_DECL_FINAL
{
public:
    ChainImageObject(Swapchain* swapchain);
    ~ChainImageObject();

    SwapchainImage* image() const;

    ImageView* imageView() const;

    void reset(const VkImage& handle);

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};


class CombinedImageObject Q_DECL_FINAL
{
    Q_DISABLE_COPY(CombinedImageObject)

public:
    CombinedImageObject(Device* device);

    ~CombinedImageObject();

    void reset(
        const VkImageType& type,
        const VkFormat& format,
        const VkImageAspectFlags& aspectMask,
        const VkImageUsageFlags& usage,
        const VkImageViewType& viewType,
        const VkImageLayout& layout,
        const VkMemoryPropertyFlags& memoryFlags,
        const VkImageTiling& tiling,
        const VkSampleCountFlagBits& samples,
        bool mipped);

    Image* image() const;

    ImageView* imageView() const;
    ImageView* planarImageView() const; //FIXME clumsy

    Sampler* sampler() const;

    DeviceMemory* memory() const;

    void resize(const VkExtent3D& size);
    void resetView();

    VkImageType type() const;

    VkFormat format() const;

    VkImageAspectFlags aspectMask() const;

    void write(std::ptrdiff_t offset, const void* data, std::size_t size);

    VkImageLayout imageLayout() const;

    QImage readImage() const;

    bool isMipped() const;

private: //FIXME d-pointer
    Device* _device;

    VkImageType _type;
    VkFormat _format;
    VkImageAspectFlags _aspectMask;
    VkImageUsageFlags _usage;
    VkImageViewType _viewType;
    VkImageLayout _layout;
    VkMemoryPropertyFlags _memoryFlags;
    VkImageTiling _tiling;
    VkSampleCountFlagBits _samples;
    bool _mipped;

    core::ReferenceObjectList<Image>::ObjectRef _image;
    core::ReferenceObjectList<DeviceMemory>::ObjectRef _memory;
    core::ReferenceObjectList<ImageView>::ObjectRef _view;
    core::ReferenceObjectList<ImageView>::ObjectRef _planarView;
    core::ReferenceObjectList<Sampler>::ObjectRef _sampler;

    VkExtent3D _currentSize;
    bool _isValid;
};

} // namespace vk
} // namespace render
} // namespace engine
