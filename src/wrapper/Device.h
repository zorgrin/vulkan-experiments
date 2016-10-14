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

class PhysicalDevice;
class CommandPool;
class CommandBuffer;
class Queue;
class DeviceMemory;
class Fence;
class Semaphore;
class Fence;
class ShaderModule;
class PipelineCache;
class RenderPass;
class ShaderModule;
class Buffer;
class PipelineLayout;
class DescriptorSetLayout;
class Swapchain;
class Surface;
class Framebuffer;
class Image;
class ImageView;
class DescriptorPool;
class Sampler;

class Device Q_DECL_FINAL: public Handle<VkDevice>, public CreationInfo<VkDeviceCreateInfo>
{
    Q_DISABLE_COPY(Device)
private:
    explicit Device(PhysicalDevice* physicalDevice, std::uint32_t queueFamilyIndex);
    ~Device();

    friend class PhysicalDevice;

public:

    PhysicalDevice* physicalDevice() const;

    std::uint32_t queueFamilyIndex() const;

    Queue defaultQueue() const;

    [[deprecated]] void present(VkQueue queue, const VkPresentInfoKHR &info); //TODO move to extensions

public:
    core::ReferenceObjectList<CommandPool>::ObjectRef createCommandPool();

    core::ReferenceObjectList<DescriptorPool>::ObjectRef createDescriptorPool(unsigned maxSets, const std::map<VkDescriptorType, std::uint32_t>& typeSlots);

    core::ReferenceObjectList<PipelineCache>::ObjectRef createPipelineCache();

    core::ReferenceObjectList<RenderPass>::ObjectRef createRenderPass(const std::vector<VkAttachmentDescription>& attachments, const std::vector<VkSubpassDescription>& subpasses, const std::vector<VkSubpassDependency>& dependency);

//    Fence* createFence();
//    void destroyFence(Fence* fence);

    core::ReferenceObjectList<Semaphore>::ObjectRef createSemaphore();

    core::ReferenceObjectList<Fence>::ObjectRef createFence();

    core::ReferenceObjectList<ShaderModule>::ObjectRef createShaderModule(const std::string &filename);

    core::ReferenceObjectList<DeviceMemory>::ObjectRef allocate(std::size_t size, unsigned memoryIndex);

    core::ReferenceObjectList<Buffer>::ObjectRef createBuffer(std::size_t size, const VkBufferUsageFlagBits &usage);

    core::ReferenceObjectList<PipelineLayout>::ObjectRef createPipelineLayout(const std::vector<VkDescriptorSetLayout> &layouts);

    core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings);

    core::ReferenceObjectList<Swapchain>::ObjectRef createSwapchain(const Surface* surface, unsigned images);

    core::ReferenceObjectList<Framebuffer>::ObjectRef createFramebuffer(RenderPass* renderPass, const std::vector<ImageView*> &attachments, const VkExtent2D &size, unsigned layers);

    core::ReferenceObjectList<Image>::ObjectRef createImage(const VkImageType &type, const VkFormat &format, const VkExtent3D &size, const VkImageLayout &layout, const VkImageTiling &tiling, const VkImageUsageFlags &usage, const VkSampleCountFlagBits& samples, bool mipped);

    core::ReferenceObjectList<Sampler>::ObjectRef createSampler();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

} // namespace vk
} // namespace render
} // namespace engine

