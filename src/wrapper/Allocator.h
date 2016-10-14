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

#include "defines.h"

#include <functional>

namespace engine
{
namespace render
{
namespace vk
{

class Allocator
{
public:
    Allocator();
    virtual ~Allocator();

    void trace(std::function<void()> &function);

    virtual const VkAllocationCallbacks* callbackHandler() const;

    virtual void* allocate(void* userData, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope);
    virtual void* reallocate(void* userData, void* original, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope);
    virtual void free(void* userData, void* memory);
    virtual void allocationNotification(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
    virtual void freeNotification(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

class MemoryManager Q_DECL_FINAL
{
    Q_DISABLE_COPY(MemoryManager);
public:
    ~MemoryManager();

    static MemoryManager& instance();

    Allocator* allocator() const;

private:
    MemoryManager();

private:
    std::unique_ptr<Allocator> m_allocator;    
};

} // namespace vk
} // namespace render
} // namespace engine
