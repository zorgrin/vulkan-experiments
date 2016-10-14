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

#include "RenderObjectGroup.h"
#include "RenderObject.h"
#include "SimpleBufferObject.h"
#include "ImageObject.h"
#include "hardcode.h"

#include <wrapper/Device.h>
#include <wrapper/DescriptorPool.h>
#include <wrapper/DescriptorSetLayout.h>
#include <wrapper/DescriptorSet.h>
#include <wrapper/RenderPass.h>
#include <wrapper/PipelineLayout.h>
#include <wrapper/PipelineCache.h>
#include <wrapper/Pipeline.h>
#include <wrapper/ShaderModule.h>
#include <wrapper/Buffer.h>
#include <wrapper/ImageView.h>
#include <wrapper/Sampler.h>
#include <wrapper/exceptions.hpp>

namespace engine
{
namespace render
{
namespace vk
{

struct RenderObjectGroup::Private
{
    Device* device;
    DescriptorSetLayout* attachmentLayout;
    RenderObjectGroup::DescriptorSetFormat objectSetFormat;
    bool finalized;

    core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef groupLayout;
    core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef objectLayout;

    core::ReferenceObjectList<DescriptorPool>::ObjectRef descriptorPool;
    core::ReferenceObjectList<DescriptorSet>::ObjectRef groupSet;

    core::ReferenceObjectList<PipelineLayout>::ObjectRef pipelineLayout;

    std::list<RenderObject*> objects;

    std::map<RenderObjectGroup::DescriptorSetFormat::BindingPoint, std::shared_ptr<SimpleBufferObject>> groupUniforms;
    std::map<RenderObjectGroup::DescriptorSetFormat::BindingPoint, std::shared_ptr<SimpleBufferObject>> groupStorages;
    std::map<RenderObjectGroup::DescriptorSetFormat::BindingPoint, std::shared_ptr<CombinedImageObject>> groupImages;

    Private(
        Device* device,
        DescriptorSetLayout* attachmentLayout,
        const RenderObjectGroup::DescriptorSetFormat& objectSetFormat):
            device(device),
            attachmentLayout(attachmentLayout),
            objectSetFormat(objectSetFormat),
            finalized(false)
    {

    }
};

RenderObjectGroup::RenderObjectGroup(
    Device* device,
    DescriptorSetLayout* attachmentLayout,
    const DescriptorSetFormat& groupSetFormat,
    const DescriptorSetFormat& objectSetFormat):
    m_d(new Private(device, attachmentLayout, objectSetFormat))
{
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (auto i: groupSetFormat.uniforms)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_ALL; //FIXME
            binding.pImmutableSamplers = nullptr;

            m_d->groupUniforms[i].reset(new SimpleBufferObject(device));

            bindings.push_back(binding);
        }

        for (auto i: groupSetFormat.storages)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_ALL; //FIXME
            binding.pImmutableSamplers = nullptr;

            m_d->groupStorages[i].reset(new SimpleBufferObject(device));

            bindings.push_back(binding);
        }

        for (auto i: groupSetFormat.combinedImages)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //FIXME
            binding.pImmutableSamplers = nullptr;

            m_d->groupImages[i].reset(new CombinedImageObject(device));

            bindings.push_back(binding);
        }

        if (!bindings.empty())
            m_d->groupLayout = m_d->device->createDescriptorSetLayout(bindings);
    }

    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (auto i: objectSetFormat.uniforms)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_ALL; //FIXME
            binding.pImmutableSamplers = nullptr;

            bindings.push_back(binding);
        }

        for (auto i: objectSetFormat.storages)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_ALL; //FIXME
            binding.pImmutableSamplers = nullptr;

            bindings.push_back(binding);
        }

        for (auto i: objectSetFormat.combinedImages)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = i;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //FIXME
            binding.pImmutableSamplers = nullptr;

            bindings.push_back(binding);
        }

        if (!bindings.empty())
            m_d->objectLayout = m_d->device->createDescriptorSetLayout(bindings);
    }


    {
        std::map<VkDescriptorType, unsigned> typeSlots;
        typeSlots[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = static_cast<unsigned>(groupSetFormat.uniforms.size());
        typeSlots[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = static_cast<unsigned>(groupSetFormat.storages.size());
        typeSlots[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = static_cast<unsigned>(groupSetFormat.combinedImages.size());

        if ((groupSetFormat.uniforms.size() + groupSetFormat.storages.size() + groupSetFormat.combinedImages.size()) > 0)
        {
            m_d->descriptorPool = m_d->device->createDescriptorPool(1, typeSlots);

            m_d->groupSet = m_d->descriptorPool->createDescriptorSet(m_d->groupLayout.ptr());
        }

    }

    {
        std::vector<VkDescriptorSetLayout> layouts;
        layouts.push_back(m_d->attachmentLayout->handle());
        if (!m_d->groupLayout.isNull())
            layouts.push_back(m_d->groupLayout->handle());
        if (!m_d->objectLayout.isNull())
            layouts.push_back(m_d->objectLayout->handle());

        m_d->pipelineLayout = m_d->device->createPipelineLayout(layouts);
    }
}

RenderObjectGroup::~RenderObjectGroup()
{

}

void RenderObjectGroup::addObject(RenderObject* object)
{
    assert(m_d->finalized);
    m_d->objects.push_back(object);
}

DescriptorSetLayout* RenderObjectGroup::objectLayout() const
{
    assert(m_d->finalized);
    return m_d->objectLayout.isNull() ? nullptr : m_d->objectLayout.ptr();
}

DescriptorSet* RenderObjectGroup::groupSet() const
{
    assert(m_d->finalized);
    return m_d->groupSet.isNull() ? nullptr : m_d->groupSet.ptr();
}

std::list<RenderObject*> RenderObjectGroup::objects() const
{
    return m_d->objects;
}

PipelineLayout* RenderObjectGroup::pipelineLayout() const
{
    assert(m_d->finalized);
    return m_d->pipelineLayout.ptr();
}

void RenderObjectGroup::finalize()
{
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorImageInfo> imageDescriptors;
    std::vector<VkDescriptorBufferInfo> bufferDescriptors;

    for (auto i: m_d->groupUniforms)
    {
        VkDescriptorBufferInfo descriptor{};
        descriptor.buffer = i.second->buffer()->handle();
        descriptor.offset = 0; //FIXME externalize
        descriptor.range = VK_WHOLE_SIZE;

        bufferDescriptors.push_back(descriptor);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = m_d->groupSet->handle();
        write.dstBinding = i.first;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pImageInfo = nullptr;
        write.pBufferInfo = &bufferDescriptors.back();
        write.pTexelBufferView = nullptr;

        writes.push_back(write);
    }

    for (auto i: m_d->groupStorages)
    {
        VkDescriptorBufferInfo descriptor{};
        descriptor.buffer = i.second->buffer()->handle();
        descriptor.offset = 0; //FIXME externalize
        descriptor.range = VK_WHOLE_SIZE;

        bufferDescriptors.push_back(descriptor);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = m_d->groupSet->handle();
        write.dstBinding = i.first;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pImageInfo = nullptr;
        write.pBufferInfo = &bufferDescriptors.back();
        write.pTexelBufferView = nullptr;

        writes.push_back(write);
    }

    for (auto i: m_d->groupImages)
    {
        VkDescriptorImageInfo descriptor{};
        descriptor.sampler = i.second->sampler()->handle();
        descriptor.imageView = i.second->imageView()->handle();
        descriptor.imageLayout = i.second->imageLayout();

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = m_d->groupSet->handle();
        write.dstBinding = i.first;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageDescriptors.back();
        write.pBufferInfo = nullptr;
        write.pTexelBufferView = nullptr;

        writes.push_back(write);
    }

    if (!writes.empty())
        vkUpdateDescriptorSets(m_d->device->handle(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);

    m_d->finalized = true;
}

SimpleBufferObject* RenderObjectGroup::uniform(unsigned bindingPoint) const
{
    assert(m_d->groupUniforms.find(bindingPoint) != m_d->groupUniforms.end());
    return m_d->groupUniforms.at(bindingPoint).get();
}

SimpleBufferObject* RenderObjectGroup::storage(unsigned bindingPoint) const
{
    assert(m_d->groupStorages.find(bindingPoint) != m_d->groupStorages.end());
    return m_d->groupStorages.at(bindingPoint).get();
}

CombinedImageObject* RenderObjectGroup::combinedImage(unsigned bindingPoint) const
{
    assert(m_d->groupImages.find(bindingPoint) != m_d->groupImages.end());
    return m_d->groupImages.at(bindingPoint).get();
}

} // namespace vk
} // namespace render
} // namespace engine
