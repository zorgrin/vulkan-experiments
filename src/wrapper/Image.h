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

#include "Handle.hpp"
#include "reference-list.hpp"

namespace engine
{
namespace render
{
namespace vk
{

class Device;
class DeviceMemory;
class ImageView;

class ImageBase: public Handle<VkImage>
{
    Q_DISABLE_COPY(ImageBase)
protected:
    ImageBase(Device* device);
    virtual ~ImageBase();

public:
    core::ReferenceObjectList<ImageView>::ObjectRef createView(const VkImageViewCreateInfo &info);

    Device* device() const;

protected:
    void clearViews();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};


class SwapchainImage Q_DECL_FINAL: public ImageBase
{
    Q_DISABLE_COPY(SwapchainImage)
public:
    explicit SwapchainImage(Device* device, const VkImage& handle);
    ~SwapchainImage();
};

class Image Q_DECL_FINAL: public ImageBase, public CreationInfo<VkImageCreateInfo>
{
    Q_DISABLE_COPY(Image)
private:
    explicit Image(Device* device, const VkImageCreateInfo &info);
    ~Image();

    static Image* create(
        Device* device,
        const VkImageType& type,
        const VkFormat& format,
        const VkExtent3D& size,
        const VkImageLayout& layout,
        const VkImageTiling& tiling,
        const VkImageUsageFlags& usage,
        const VkSampleCountFlagBits& samples,
        bool mipped);

    friend class Device;

public:
    void bind(DeviceMemory* memory, std::size_t memoryOffset);
};

} // namespace vk
} // namespace render
} // namespace engine
