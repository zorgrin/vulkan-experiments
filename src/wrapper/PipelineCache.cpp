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


#include "PipelineCache.h"
#include "Device.h"
#include "Pipeline.h"
#include "Allocator.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct PipelineCache::Private
{
    Device* device;
            
    core::ReferenceObjectList<Pipeline> pipelines;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(Device* device): device(device), pipelines(friendDeleter<Pipeline>)
    {
    }
};

PipelineCache::PipelineCache(Device* device, const VkPipelineCacheCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    auto r(vkCreatePipelineCache(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

PipelineCache::~PipelineCache()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyPipelineCache(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

PipelineCache* PipelineCache::create(Device* device, const std::vector<std::uint8_t> &initialState)
{
    VkPipelineCacheCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.initialDataSize = initialState.empty() ? 0 : initialState.size();
    info.pInitialData = initialState.empty() ? nullptr : initialState.data();

    return new PipelineCache(device, info);
}

Device* PipelineCache::device() const
{
    return m_d->device;
}


core::ReferenceObjectList<Pipeline>::ObjectRef PipelineCache::createPipeline(const VkGraphicsPipelineCreateInfo &info)
{
    return m_d->pipelines.append(new Pipeline(this, info));
}

QByteArray PipelineCache::data() const
{
    std::size_t size(0);

    auto r(vkGetPipelineCacheData(m_d->device->handle(), m_handle, &size, nullptr));
    if (r != VK_INCOMPLETE)
        CHECK_VK_RESULT(r, VK_SUCCESS);

    QByteArray buffer;
    buffer.resize(static_cast<int>(size));
    r = vkGetPipelineCacheData(m_d->device->handle(), m_handle, &size, buffer.data());
    CHECK_VK_RESULT(r, VK_SUCCESS);

    return buffer;
}

} // namespace vk
} // namespace render
} // namespace engine
