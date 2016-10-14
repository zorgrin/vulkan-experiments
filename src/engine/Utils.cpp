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

#include "Utils.h"
#include "ImageObject.h"
#include "SimpleBufferObject.h"

#include <QtCore/QFile>

#include <QtGui/QImage>

#include <wrapper/exceptions.hpp>
#include <wrapper/Image.h>
#include <wrapper/Buffer.h>
#include <wrapper/DeviceMemory.h>
#include <wrapper/CommandPool.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/Queue.h>

#if defined(Q_OS_LINUX)
#include <execinfo.h>
#include <cxxabi.h>
#else
//TODO #error "OS is not supported yet"
#endif

namespace engine
{
namespace core
{

void dumpBinary(const QString& filename, const QByteArray& buffer)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(buffer);
        file.close();
    }
    else
    {
        throw core::Exception(file.errorString().toStdString());
    }
}


#if defined(Q_OS_LINUX)
std::list<std::string> debug_backtrace()
{
    std::list<std::string> result;
    constexpr unsigned depthLimit(20);
    void *array[depthLimit];
    int size;
    char **strings;

    size = backtrace(array, depthLimit);
    strings = backtrace_symbols(array, size);

    for (int i = 3; i < size; i++)
    {
        QByteArray line(strings[i]); // TODO remove Qt dependency
        line = line.mid(line.indexOf("(") + 1);
        line = line.left(line.indexOf("+"));

        int status = -1;
        char *demangledName = abi::__cxa_demangle(line, nullptr, nullptr, &status);
        if (demangledName != nullptr)
            result.push_back(std::string(demangledName));
        else
            result.push_back(strings[i] == nullptr ? std::string("") : strings[i]);
        free(demangledName);
    }

    free(strings);

    return result;
}
#else
//#error "OS is not supported yet"
#endif

} // namespace core

namespace
{

static VkAccessFlags srcAccessMaskFor(const VkImageLayout& oldLayout, const VkImageLayout& newLayout)
{
    VkAccessFlags srcAccessMask(0);

    if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        srcAccessMask = srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    return srcAccessMask;
}

static VkAccessFlags dstAccessMaskFor(const VkImageLayout& oldLayout, const VkImageLayout& newLayout)
{
    Q_UNUSED(oldLayout);
    VkAccessFlags dstAccessMask(0);

    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    return dstAccessMask;
}

}

namespace render
{
namespace vk
{

void loadTexture(CommandPool* pool, CombinedImageObject* image, const std::string& filename)
{
    if (image->type() != VK_IMAGE_TYPE_2D)
        throw core::Exception("image is not 2D");

    QImage file(QString::fromLocal8Bit(filename.c_str()));

    if (file.isNull())
        throw core::Exception("unable to open image file");

    if (file.format() == QImage::Format_RGB32) // BGR32->RGB24
        file = file.convertToFormat(QImage::Format_RGB888);
    else if (file.format() == QImage::Format_Indexed8)
        file = file.convertToFormat(QImage::Format_RGB888);


    image->resize(VkExtent3D{ static_cast<std::uint32_t>(file.width()), static_cast<std::uint32_t>(file.height()), 1});
    image->resetView();

    {
        VkFormat format(converters::convert(file.format()));
        if (format == VK_FORMAT_UNDEFINED)
            throw core::Exception("image format is not supported");

        if (image->image()->creationInfo().format != format)
        {
            auto qtFormat(converters::convert(image->image()->creationInfo().format));
            if (qtFormat == QImage::Format_Invalid)
                throw core::Exception("image format conversion is not supported");
            file = file.convertToFormat(qtFormat);
            assert(!file.isNull());
        }
    }

    QImage current(file);
    std::vector<std::uint8_t> data;

    std::vector<VkBufferImageCopy> regions;

    const unsigned totalMips(image->image()->creationInfo().mipLevels);

    for (unsigned mip = 0; mip < totalMips; ++mip)
    {
        assert(!current.isNull());

        std::size_t dataSize(static_cast<std::size_t>(current.bytesPerLine() * current.height()));
        {
            VkBufferImageCopy region{};
            region.bufferOffset = data.size();
            region.bufferRowLength = static_cast<std::uint32_t>(current.width());
            region.bufferImageHeight = static_cast<std::uint32_t>(current.height());

            region.imageSubresource.aspectMask = image->aspectMask();
            region.imageSubresource.mipLevel = mip;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

//            region.imageOffset = {}; //0,0,0

            region.imageExtent.width = static_cast<std::uint32_t>(current.width());
            region.imageExtent.height = static_cast<std::uint32_t>(current.height());
            region.imageExtent.depth = 1;

            regions.push_back(region);
        }

        data.insert(data.end(), reinterpret_cast<std::uint8_t*>(current.bits()), reinterpret_cast<std::uint8_t*>(current.bits()) + dataSize);

        current = current.scaled(current.size() / 2);
    }

    std::unique_ptr<SimpleBufferObject> stageBuffer(new SimpleBufferObject(pool->device()));
    stageBuffer->reset(data.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    stageBuffer->write(0, data.data(), data.size());

    core::ReferenceObjectList<CommandBuffer>::ObjectRef command(pool->createCommandBuffer());
    {
        auto buffer(command.ptr());
        buffer->enqueueCommand([&](CommandBuffer* buffer)
        {
            VkImageMemoryBarrier barrier{};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image->image()->handle();

            barrier.subresourceRange.aspectMask = image->aspectMask();
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = totalMips;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            vkCmdCopyBufferToImage(buffer->handle(), stageBuffer->buffer()->handle(), image->image()->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<std::uint32_t>(regions.size()), regions.data());

            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        });

        buffer->finalize();
    }
    //TODO async/TextureLoaderThread
    {
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = nullptr;

        submit.waitSemaphoreCount = 0;
        submit.pWaitSemaphores = nullptr;

        VkPipelineStageFlags stageMask(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        submit.pWaitDstStageMask = &stageMask;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = command->handlePtr();

        submit.signalSemaphoreCount = 0;
        submit.pSignalSemaphores = nullptr;

        pool->device()->defaultQueue().submit(submit, VK_NULL_HANDLE);
        pool->device()->defaultQueue().wait();
    }

    command.reset();

    stageBuffer.reset();
}

void setImageLayout(
    const ImageBase* image,
    const CommandBuffer* buffer,
    const VkImageAspectFlags& aspectMask,
    const VkImageLayout& oldLayout,
    const VkImageLayout& newLayout,
    const VkPipelineStageFlags& sourceStage,
    const VkPipelineStageFlags& destStage)
{
    assert(buffer);
    assert(image);
    assert(image->handle());
    VkImageMemoryBarrier info{};

    info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    info.pNext = nullptr;
    info.srcAccessMask = srcAccessMaskFor(oldLayout, newLayout);
    info.dstAccessMask = dstAccessMaskFor(oldLayout, newLayout);
    info.oldLayout = oldLayout;
    info.newLayout = newLayout;
    info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.image = image->handle();

    info.subresourceRange.aspectMask = aspectMask;
    info.subresourceRange.baseMipLevel = 0;
    if (dynamic_cast<const Image*>(image)) //TODO ImageBase::type()
        info.subresourceRange.levelCount = static_cast<const Image*>(image)->creationInfo().mipLevels;
    else
        info.subresourceRange.levelCount = 1;

    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(buffer->handle(), sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &info);
}

} // namespace vk
} // namespace render
} // namespace engine
