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


#include "Pipeline.h"
#include "Device.h"
#include "PipelineCache.h"
#include "Allocator.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct Pipeline::Private
{
    PipelineCache* cache;
    VkPipelineBindPoint type;

    union
    {
        VkGraphicsPipelineCreateInfo graphics;
        VkComputePipelineCreateInfo compute;
    } creationInfo;
    
    Private(PipelineCache* cache, const VkGraphicsPipelineCreateInfo& info): cache(cache), type(VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        creationInfo.graphics = info;
    }
    Private(PipelineCache* cache, const VkComputePipelineCreateInfo& info): cache(cache), type(VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        creationInfo.compute = info;
    }
};

Pipeline::Pipeline(PipelineCache* cache, const VkGraphicsPipelineCreateInfo &info): 
    Handle(VK_NULL_HANDLE), m_d(new Private(cache, info))
{
    validateStruct(info);

    auto r(vkCreateGraphicsPipelines(m_d->cache->device()->handle(), m_d->cache->handle(), 1, &info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}


Pipeline::Pipeline(PipelineCache* cache, const VkComputePipelineCreateInfo &info): 
    Handle(VK_NULL_HANDLE), m_d(new Private(cache, info))
{
    validateStruct(info);

    auto r(vkCreateComputePipelines(m_d->cache->device()->handle(), m_d->cache->handle(), 1, &info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

Pipeline::~Pipeline()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyPipeline(m_d->cache->device()->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

PipelineCache* Pipeline::cache() const
{
    return m_d->cache;
}

template <>
VkGraphicsPipelineCreateInfo Pipeline::creationInfo<VkGraphicsPipelineCreateInfo>() const
{
    return m_d->creationInfo.graphics;
}

template <>
VkComputePipelineCreateInfo Pipeline::creationInfo<VkComputePipelineCreateInfo>() const
{
    return m_d->creationInfo.compute;
}

VkPipelineBindPoint Pipeline::type() const
{
    return m_d->type;
}

} // namespace vk
} // namespace render
} // namespace engine
