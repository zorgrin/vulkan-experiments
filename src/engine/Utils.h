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

#include <QtGui/QImage>

class QByteArray;

namespace engine
{
namespace core
{

void dumpBinary(const QString& filename, const QByteArray& buffer);

std::list<std::string> debug_backtrace();

} // namespace core

namespace render
{
namespace vk
{

class CombinedImageObject;
class DeviceMemory;
class ImageBase;
class CommandBuffer;
class CommandPool;

void loadTexture(CommandPool* pool, CombinedImageObject* image, const std::string& filename);

[[deprecated]] void setImageLayout(
    const ImageBase* image,
    const CommandBuffer* buffer,
    const VkImageAspectFlags& aspectMask,
    const VkImageLayout& oldLayout,
    const VkImageLayout& newLayout,
    const VkPipelineStageFlags& sourceStage,
    const VkPipelineStageFlags& destStage);

namespace converters
{

inline VkFormat convert(const QImage::Format& format)
{
    switch (format)
    {
    case QImage::Format_ARGB32:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case QImage::Format_RGB888:
        return VK_FORMAT_R8G8B8_UNORM;
    case QImage::Format_RGBA8888:
        return VK_FORMAT_R8G8B8A8_UNORM;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

inline QImage::Format convert(const VkFormat& format)
{
    switch (format)
    {
    case VK_FORMAT_B8G8R8A8_UNORM:
        return QImage::Format_ARGB32;
    case VK_FORMAT_R8G8B8_UNORM:
        return QImage::Format_RGB888;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return QImage::Format_RGBA8888;
    default:
        return QImage::Format_Invalid;
    }
}

inline VkExtent2D toExtent2D(const VkExtent3D& extent)
{
    return VkExtent2D{extent.width, extent.height};
}

} // namespace converters
} // namespace vk
} // namespace render
} // namespace engine
