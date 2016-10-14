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


#include "Device.h"

#include "Allocator.h"
#include "Instance.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DescriptorPool.h"
#include "DeviceMemory.h"
#include "Buffer.h"
#include "PhysicalDevice.h"
#include "Queue.h"
#include "Swapchain.h"
#include "PipelineCache.h"
#include "PipelineLayout.h"
#include "DescriptorSetLayout.h"
#include "RenderPass.h"
#include "ShaderModule.h"
#include "Fence.h"
#include "Semaphore.h"
#include "Framebuffer.h"
#include "Image.h"
#include "Sampler.h"
#include "exceptions.hpp"

#include <set>
#include <algorithm>

namespace engine
{
namespace render
{
namespace vk
{

struct Device::Private
{
    PFN_vkQueuePresentKHR QueuePresentKHR = nullptr;

    PhysicalDevice* physicalDevice;    
    std::uint32_t queueFamilyIndex;
    
    std::vector<Queue> queues;
    core::ReferenceObjectList<CommandPool> commandPools;
    core::ReferenceObjectList<DescriptorPool> descriptorPools;
    core::ReferenceObjectList<PipelineCache> caches;
    core::ReferenceObjectList<RenderPass> renderPasses;
    core::ReferenceObjectList<Fence> fences;
    core::ReferenceObjectList<Semaphore> semaphores;
    core::ReferenceObjectList<ShaderModule> shaderModules;
    core::ReferenceObjectList<DeviceMemory> memoryBlocks;
    core::ReferenceObjectList<Buffer> buffers;
    core::ReferenceObjectList<PipelineLayout> pipelineLayouts;
    core::ReferenceObjectList<DescriptorSetLayout> descriptorSetLayouts;
    core::ReferenceObjectList<Swapchain> swapchains;
    core::ReferenceObjectList<Framebuffer> framebuffers;
    core::ReferenceObjectList<Image> images;
    core::ReferenceObjectList<Sampler> sampler;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(PhysicalDevice* physicalDevice, std::uint32_t queueFamilyIndex): 
        physicalDevice(physicalDevice),
        queueFamilyIndex(queueFamilyIndex),
        commandPools(friendDeleter<CommandPool>),
        descriptorPools(friendDeleter<DescriptorPool>),
        caches(friendDeleter<PipelineCache>),
        renderPasses(friendDeleter<RenderPass>),
        fences(friendDeleter<Fence>),
        semaphores(friendDeleter<Semaphore>),
        shaderModules(friendDeleter<ShaderModule>),
        memoryBlocks(friendDeleter<DeviceMemory>),
        buffers(friendDeleter<Buffer>),
        pipelineLayouts(friendDeleter<PipelineLayout>),
        descriptorSetLayouts(friendDeleter<DescriptorSetLayout>),
        swapchains(friendDeleter<Swapchain>),
        framebuffers(friendDeleter<Framebuffer>),
        images(friendDeleter<Image>),
        sampler(friendDeleter<Sampler>)
    {
        QueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetInstanceProcAddr(physicalDevice->instance()->handle(), "vkQueuePresentKHR"));
        if (QueuePresentKHR == nullptr)
            throw core::Exception("unable to load library symbols");
    }
};

Device::Device(PhysicalDevice* physicalDevice, std::uint32_t queueFamilyIndex):
    Handle(VK_NULL_HANDLE),
    CreationInfo(),
    m_d(new Private(physicalDevice, queueFamilyIndex))
{
    auto queueFamilies(physicalDevice->queueFamilies());
    assert(queueFamilyIndex < queueFamilies.size());
    assert(queueFamilies.at(queueFamilyIndex).queueFlags & VK_QUEUE_GRAPHICS_BIT);

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = nullptr;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1; //TODO use multiple queues?
    const float priority(0.0);
    queueInfo.pQueuePriorities = &priority;

    m_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    m_info.pNext = nullptr;
    m_info.queueCreateInfoCount = 1; //TODO use multiple queues?
    m_info.pQueueCreateInfos = &queueInfo;

    std::vector<VkExtensionProperties> supportedExtensions;
    {
        std::uint32_t count(0);
        auto r(vkEnumerateDeviceExtensionProperties(m_d->physicalDevice->handle(), nullptr, &count, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);

        supportedExtensions.resize(count);

        r = vkEnumerateDeviceExtensionProperties(m_d->physicalDevice->handle(), nullptr, &count, supportedExtensions.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);
        if (supportedExtensions.empty())
            qDebug() << "no device extensions";
        for (auto j: supportedExtensions)
            qDebug() << "supported devices extension" << j.extensionName << j.specVersion;

    }

    std::vector<const char*> required;
    required.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    {
        std::set <std::string> requiredSet(required.begin(), required.end());

        std::set <std::string> supported;
        for (auto i: supportedExtensions)
            supported.insert(i.extensionName);

        std::set<std::string> unsupported;

        std::set_difference(
            requiredSet.begin(), requiredSet.end(),
            supported.begin(), supported.end(),
            std::inserter(unsupported, unsupported.end()));

        for (auto i: unsupported)
            qCritical("[FATAL] extension %s is not supported", i.c_str());

        if (!unsupported.empty())
            throw core::Exception("critical extension is not supported");
    }

    m_info.enabledExtensionCount = static_cast<std::uint32_t>(required.size());
    m_info.ppEnabledExtensionNames = required.data();

//    auto forcedFeatures(physicalDevice->features());
//    m_info.pEnabledFeatures = &forcedFeatures;
    m_info.pEnabledFeatures = nullptr;

    validateStruct(m_info);

    auto r(vkCreateDevice(physicalDevice->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);

    {
        const unsigned queueIndex(0); //TODO multiple
        VkQueue queue;
        vkGetDeviceQueue(m_handle, queueFamilyIndex, queueIndex, &queue);
        assert(queue != nullptr);
        m_d->queues.push_back(Queue(queue));
    }
}

Device::~Device()
{
    m_d->renderPasses.clear();
    m_d->shaderModules.clear();
    m_d->pipelineLayouts.clear();
    m_d->descriptorSetLayouts.clear();
    m_d->framebuffers.clear();
    m_d->swapchains.clear();
    m_d->semaphores.clear();
    m_d->buffers.clear();
    m_d->memoryBlocks.clear();
    m_d->descriptorPools.clear();
    m_d->commandPools.clear();
    m_d->caches.clear();
    m_d->queues.clear();

    if (m_handle != VK_NULL_HANDLE)
        vkDestroyDevice(m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

PhysicalDevice* Device::physicalDevice() const
{
    return m_d->physicalDevice;
}

std::uint32_t Device::queueFamilyIndex() const
{
    return m_d->queueFamilyIndex;
}

void Device::present(VkQueue queue, const VkPresentInfoKHR &info)
{
    auto r(m_d->QueuePresentKHR(queue, &info));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

core::ReferenceObjectList<CommandPool>::ObjectRef Device::createCommandPool()
{
    return m_d->commandPools.append(CommandPool::create(this));
}

core::ReferenceObjectList<DescriptorPool>::ObjectRef Device::createDescriptorPool(unsigned maxSets, const std::map<VkDescriptorType, std::uint32_t>& typeSlots)
{
    return m_d->descriptorPools.append(DescriptorPool::create(this, maxSets, typeSlots));
}

core::ReferenceObjectList<PipelineCache>::ObjectRef Device::createPipelineCache()
{
    return m_d->caches.append(PipelineCache::create(this));
}

core::ReferenceObjectList<RenderPass>::ObjectRef Device::createRenderPass(const std::vector<VkAttachmentDescription>& attachments, const std::vector<VkSubpassDescription>& subpasses, const std::vector<VkSubpassDependency>& dependency)
{
    return m_d->renderPasses.append(RenderPass::create(this, attachments, subpasses, dependency));
}

core::ReferenceObjectList<ShaderModule>::ObjectRef Device::createShaderModule(const std::string &filename)
{
    return m_d->shaderModules.append(ShaderModule::create(this, filename));
}

core::ReferenceObjectList<PipelineLayout>::ObjectRef Device::createPipelineLayout(const std::vector<VkDescriptorSetLayout> &layouts)
{
    return m_d->pipelineLayouts.append(PipelineLayout::create(this, layouts));
}

core::ReferenceObjectList<Semaphore>::ObjectRef Device::createSemaphore()
{
    return m_d->semaphores.append(Semaphore::create(this));
}

core::ReferenceObjectList<Fence>::ObjectRef Device::createFence()
{
    return m_d->fences.append(Fence::create(this));
}

core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef Device::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
    return m_d->descriptorSetLayouts.append(DescriptorSetLayout::create(this, bindings));
}

core::ReferenceObjectList<DeviceMemory>::ObjectRef Device::allocate(std::size_t size, unsigned memoryIndex)
{
    return m_d->memoryBlocks.append(DeviceMemory::create(this, size, memoryIndex));
}

core::ReferenceObjectList<Buffer>::ObjectRef Device::createBuffer(std::size_t size, const VkBufferUsageFlagBits &usage)
{
    return m_d->buffers.append(Buffer::create(this, size, usage));
}

Queue Device::defaultQueue() const
{
    assert(m_d->queues.size() == 1);
    return m_d->queues.front();
}

core::ReferenceObjectList<Swapchain>::ObjectRef Device::createSwapchain(const Surface* surface, unsigned images)
{
    return m_d->swapchains.append(new Swapchain(this, surface, images));
}

core::ReferenceObjectList<Framebuffer>::ObjectRef Device::createFramebuffer(RenderPass* renderPass, const std::vector<ImageView*> &attachments, const VkExtent2D &size, unsigned layers)
{
    assert(!attachments.empty());
    return m_d->framebuffers.append(Framebuffer::create(renderPass, attachments, size, layers));
}

core::ReferenceObjectList<Image>::ObjectRef Device::createImage(const VkImageType &type, const VkFormat &format, const VkExtent3D &size, const VkImageLayout &layout, const VkImageTiling& tiling, const VkImageUsageFlags& usage, const VkSampleCountFlagBits& samples, bool mipped)
{
    return m_d->images.append(Image::create(this, type, format, size, layout, tiling, usage, samples, mipped));
}

core::ReferenceObjectList<Sampler>::ObjectRef Device::createSampler()
{
    return m_d->sampler.append(Sampler::create(this));
}

} // namespace vk
} // namespace render
} // namespace engine
