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

#include "RenderObject.h"
#include "Utils.h"
#include "SimpleBufferObject.h"
#include "ImageObject.h"
#include "RenderBatch.h"

#include <wrapper/CommandPool.h>
#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/Buffer.h>
#include <wrapper/DeviceMemory.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/DescriptorPool.h>
#include <wrapper/DescriptorSet.h>
#include <wrapper/DescriptorSetLayout.h>
#include <wrapper/PipelineLayout.h>
#include <wrapper/Sampler.h>
#include <wrapper/ImageView.h>
#include <wrapper/RenderPass.h>
#include <wrapper/exceptions.hpp>

namespace engine
{
namespace render
{
namespace vk
{

RenderObject::RenderObject(Device* device, DescriptorSetLayout* objectLayout, const InputData& input):
    _device(device),
    _objectLayout(objectLayout),
    _input(input),
    _instanceCount(1),
    _indirectData{}
{
    init();
}

RenderObject::~RenderObject()
{
    _objectSet.reset();
}

void RenderObject::init()
{
    {
        std::map<VkDescriptorType, unsigned> typeSlots;

        typeSlots[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = static_cast<unsigned>(_input.images.size());
        typeSlots[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = static_cast<unsigned>(_input.storages.size());
        typeSlots[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = static_cast<unsigned>(_input.uniforms.size());

        constexpr unsigned maxSets(1); //FIXME hardcoded

        if ((_input.uniforms.size() + _input.storages.size() + _input.images.size()) > 0)
        {
            _descriptorPool = _device->createDescriptorPool(maxSets, typeSlots);

            _objectSet = _descriptorPool->createDescriptorSet(_objectLayout);
        }

        std::vector<VkWriteDescriptorSet> writes;

        std::vector<VkDescriptorBufferInfo> uniforms;
        for (auto i: _input.uniforms)
        {
            const unsigned index(static_cast<unsigned>(uniforms.size()));
            {
                VkDescriptorBufferInfo descriptor{};

                descriptor.buffer = i.second->buffer()->handle();
                descriptor.offset = 0;
                descriptor.range = VK_WHOLE_SIZE;

                uniforms.push_back(descriptor);
            }
            {
                VkWriteDescriptorSet info{};
                info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                info.pNext = nullptr;
                info.dstSet = _objectSet->handle();
                info.dstBinding = i.first;
                info.dstArrayElement = 0;
                info.descriptorCount = 1;
                info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                info.pImageInfo = nullptr;
                info.pBufferInfo = uniforms.data() + index;
                info.pTexelBufferView = nullptr;

                validateStruct(info);
                writes.push_back(info);
            }

        }

        std::vector<VkDescriptorBufferInfo> storages;
        for (auto i: _input.storages)
        {
            const std::size_t index(storages.size());
            {
                VkDescriptorBufferInfo descriptor{};

                descriptor.buffer = i.second->buffer()->handle();
                descriptor.offset = 0;
                descriptor.range = VK_WHOLE_SIZE;

                storages.push_back(descriptor);
            }
            {
                VkWriteDescriptorSet info{};
                info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                info.pNext = nullptr;
                info.dstSet = _objectSet->handle();
                info.dstBinding = i.first;
                info.dstArrayElement = 0;
                info.descriptorCount = 1;
                info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                info.pImageInfo = nullptr;
                info.pBufferInfo = storages.data() + index;
                info.pTexelBufferView = nullptr;

                validateStruct(info);
                writes.push_back(info);
            }
        }

        std::vector<VkDescriptorImageInfo> images;
        for (auto i: _input.images)
        {
            const std::size_t index(images.size());
            {
                VkDescriptorImageInfo descriptor{};
                descriptor.sampler = i.second->sampler()->handle();
                descriptor.imageView = i.second->imageView()->handle();
                descriptor.imageLayout = i.second->imageLayout();

                images.push_back(descriptor);
            }
            {
                VkWriteDescriptorSet info{};
                info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                info.pNext = nullptr;
                info.dstSet = _objectSet->handle();
                info.dstBinding = i.first;
                info.dstArrayElement = 0;
                info.descriptorCount = 1;
                info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                info.pImageInfo = images.data() + index;
                info.pBufferInfo = nullptr;
                info.pTexelBufferView = nullptr;

                validateStruct(info);
                writes.push_back(info);
            }
        }

        if (!writes.empty())
            vkUpdateDescriptorSets(_device->handle(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    {

        _indirectData.instanceCount = _instanceCount;
        _indirectData.indexCount = _input.indexCount;
        _indirectData.firstIndex = 0;
        _indirectData.vertexOffset = 0;
        _indirectData.firstInstance = 0;

        const std::size_t size(sizeof(_indirectData));
        _indirect.reset(new SimpleBufferObject(_device));
        _indirect->reset(size, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        _indirect->write(0, &_indirectData, size);
    }
}

void RenderObject::buildSubpass(CommandBuffer* buffer)
{
    if (_indirectData.instanceCount > 0)
    {
        for (auto i: _input.attributes)
        {
            VkDeviceSize offset(0);
            VkBuffer handle(i.second->buffer()->handle());

            vkCmdBindVertexBuffers(buffer->handle(), i.first, 1, &handle, &offset);
        }

        vkCmdBindIndexBuffer(buffer->handle(), _input.indexBuffer->buffer()->handle(), 0, VK_INDEX_TYPE_UINT32); //TODO UINT16

        vkCmdDrawIndexedIndirect(buffer->handle(), _indirect->buffer()->handle(), 0, 1, sizeof(VkDrawIndexedIndirectCommand)); //TODO multi draw
    //    vkCmdDrawIndexed(buffer->handle(), _indices, _instanceCount, 0, 0, 0);
    }
}

void RenderObject::setInstanceCount(unsigned count)
{
    _indirectData.instanceCount = _instanceCount = count;
    {
        _indirect->write(0, &_indirectData, sizeof(_indirectData)); //TODO optimize
    }

}

DescriptorSet* RenderObject::descriptorSet() const
{
    return _objectSet.isNull() ? nullptr : _objectSet.ptr();
}

} // namespace vk
} // namespace render
} // namespace engine
