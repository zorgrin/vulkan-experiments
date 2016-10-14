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


#include "Allocator.h"

#include <mutex>
namespace engine
{
namespace render
{
namespace vk
{

namespace
{
#pragma pack(push, 1)
struct MemoryChunk
{
    std::uint32_t magic;
    std::uint8_t* allocatedPtr;
    std::size_t alignment;
    std::size_t size;
    VkSystemAllocationScope scope;
};

#pragma pack(pop)

static void* allocateHandler(void* userData, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope)
{
    return static_cast<Allocator*>(userData)->allocate(userData, size, alignment, scope);
}

static void freeHandler(void* userData, void* memory)
{
    return static_cast<Allocator*>(userData)->free(userData, memory);
}

void* reallocateHandler(void* userData, void* original, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope)
{
    return static_cast<Allocator*>(userData)->reallocate(userData, original, size, alignment, scope);
}

void allocationNotificationHandler(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
    return static_cast<Allocator*>(userData)->allocationNotification(userData, size, type, scope);
}

void freeNotificationHandler(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
    return static_cast<Allocator*>(userData)->freeNotification(userData, size, type, scope);
}

}

struct Allocator::Private
{
    bool enabled;
    VkAllocationCallbacks callbacks;
    bool traceEnabled;

    using RequestedPointer = void*;
    using ChunkDataPointer = void*;

    std::map<RequestedPointer, ChunkDataPointer> debugAllocatedChunks;

    
    Private(): enabled(false), callbacks{}, traceEnabled(false)
    {
    }
};

Allocator::Allocator(): m_d(new Private())
{
    m_d->callbacks.pUserData = this;
    m_d->callbacks.pfnAllocation = allocateHandler;
    m_d->callbacks.pfnReallocation = reallocateHandler;
    m_d->callbacks.pfnFree = freeHandler;
    m_d->callbacks.pfnInternalAllocation = allocationNotificationHandler;
    m_d->callbacks.pfnInternalFree = freeNotificationHandler;
}

Allocator::~Allocator()
{
    assert(m_d->debugAllocatedChunks.empty());
}

void Allocator::trace(std::function<void()> &function)
{
    bool old(m_d->traceEnabled);
    m_d->traceEnabled = true;
    function();
    m_d->traceEnabled = old;
}

const VkAllocationCallbacks* Allocator::callbackHandler() const
{
    return m_d->enabled ? &m_d->callbacks : nullptr;
}

void* Allocator::allocate(void* userData, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope)
{
    assert(userData == this);
    assert(alignment > 0);
    if (m_d->traceEnabled)
        qDebug() << __FUNCTION__ << size << alignment << scope;

    std::uint8_t* result(new std::uint8_t[size + sizeof(MemoryChunk) + alignment - 1]);
    if (result == nullptr)
        throw std::bad_alloc();

    std::uint8_t* allocatedPtr(result);
    result += sizeof(MemoryChunk) + alignment - 1;
    result -= reinterpret_cast<std::size_t>(result) % alignment;

    MemoryChunk* chunk(reinterpret_cast<MemoryChunk*>(result - sizeof(MemoryChunk)));
    assert((reinterpret_cast<std::uint8_t*>(chunk) - allocatedPtr) >= 0);
    chunk->magic = 0xDEADBEEF;
    chunk->allocatedPtr = allocatedPtr;
    chunk->alignment = alignment;
    chunk->size = size;
    chunk->scope = scope;

    m_d->debugAllocatedChunks[result] = chunk;

    return result;
}

void* Allocator::reallocate(void* userData, void* original, std::size_t size, std::size_t alignment, VkSystemAllocationScope scope)
{
    TRACE();
    assert(userData == this);
    if (m_d->traceEnabled)
        qDebug() << __FUNCTION__ << original << size << alignment << scope;

    std::uint8_t* ptr(static_cast<std::uint8_t*>(original));
    std::uint8_t* newPtr(static_cast<std::uint8_t*>(allocate(userData, size, alignment, scope)));

    std::copy(ptr, ptr + size, newPtr); //FIXME C4996

    MemoryChunk* chunk(reinterpret_cast<MemoryChunk*>(static_cast<std::uint8_t*>(original) - sizeof(MemoryChunk)));
    assert(chunk->magic == 0xDEADBEEF);
    assert(chunk->alignment == alignment);
    qDebug() << "realloc" << chunk->size << "->" << size;

    delete[] chunk->allocatedPtr;

    return newPtr;
}

void Allocator::free(void* userData, void* memory)
{
    assert(userData == this);
    assert(m_d->debugAllocatedChunks.find(memory) != m_d->debugAllocatedChunks.end());
    if (m_d->traceEnabled)
        qDebug() << __FUNCTION__ << memory;

    MemoryChunk* chunk(reinterpret_cast<MemoryChunk*>(static_cast<std::uint8_t*>(memory) - sizeof(MemoryChunk)));
    assert(chunk->magic == 0xDEADBEEF);

    m_d->debugAllocatedChunks.erase(memory);

    delete[] chunk->allocatedPtr;
}

void Allocator::allocationNotification(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
    assert(userData == this);
    qDebug() << __FUNCTION__ << size << type << scope;
}

void Allocator::freeNotification(void* userData, std::size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
    assert(userData == this);
    qDebug() << __FUNCTION__ << size << type << scope;
}

////////////////////////////////////////////////////////////////////////////////
MemoryManager::MemoryManager(): m_allocator(new Allocator())
{

}

MemoryManager::~MemoryManager()
{

}

Allocator* MemoryManager::allocator() const
{
    return m_allocator.get();
}

MemoryManager& MemoryManager::instance()
{
    static std::mutex mutex;
    {
        { //FIXME debug
            if (!mutex.try_lock())
                qDebug() << "multithreaded call attempt, not used now";
            else
                mutex.unlock();
        }

        std::lock_guard<std::mutex> locker(mutex);
        static std::unique_ptr<MemoryManager> singleton(new MemoryManager());
        return *singleton.get();
    }
}

} // namespace vk
} // namespace render
} // namespace engine
