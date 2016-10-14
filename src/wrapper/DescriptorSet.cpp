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


#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DescriptorPool.h"
#include "Device.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct DescriptorSet::Private
{    
    DescriptorPool* pool;
    const DescriptorSetLayout* layout;
    
    Private(DescriptorPool* pool, const DescriptorSetLayout* layout):
        pool(pool), layout(layout)
    {

    }
};

DescriptorSet::DescriptorSet(DescriptorPool* pool, const DescriptorSetLayout* layout):
    Handle(VK_NULL_HANDLE), CreationInfo(), m_d(new Private(pool, layout))
{
    m_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    m_info.pNext = nullptr;
    m_info.descriptorPool = m_d->pool->handle();
    m_info.descriptorSetCount = 1;
    m_info.pSetLayouts = layout->handlePtr();

    validateStruct(m_info);

    auto r(vkAllocateDescriptorSets(m_d->pool->device()->handle(), &m_info, &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

DescriptorSet::~DescriptorSet()
{
    if (m_handle != VK_NULL_HANDLE)
        vkFreeDescriptorSets(m_d->pool->device()->handle(), m_d->pool->handle(), 1, &m_handle);
}

DescriptorPool* DescriptorSet::pool() const
{
    return m_d->pool;
}

const DescriptorSetLayout* DescriptorSet::layout() const
{
    return m_d->layout;
}

} // namespace vk
} // namespace render
} // namespace engine
