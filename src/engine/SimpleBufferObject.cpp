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

#include "SimpleBufferObject.h"

#include <wrapper/PhysicalDevice.h>
#include <wrapper/Device.h>
#include <wrapper/DeviceMemory.h>
#include <wrapper/Buffer.h>

namespace engine
{
namespace render
{
namespace vk
{

SimpleBufferObject::SimpleBufferObject(Device* device):
    _device(device)
{
}

SimpleBufferObject::~SimpleBufferObject()
{
    _buffer.reset();
    _memory.reset();
}

void SimpleBufferObject::reset(std::size_t size, const VkBufferUsageFlagBits& usage, const VkMemoryPropertyFlags& memoryFlags)
{
    _buffer = _device->createBuffer(size, usage);
    auto requirements(DeviceMemory::requirements(_device, _buffer->handle()));
    _memory = _device->allocate(requirements.size, _device->physicalDevice()->memoryFor(requirements, memoryFlags));
    _buffer->bind(_memory.ptr(), 0);
}

Buffer* SimpleBufferObject::buffer() const
{
    return _buffer.ptr();
}

void SimpleBufferObject::write(std::ptrdiff_t offset, const void* data, std::size_t size)
{
    _memory->map(offset, size);
    auto ptr(reinterpret_cast<const std::uint8_t*>(data));
    std::copy(ptr, ptr + size, _memory->data<std::uint8_t>());
    _memory->unmap();
}

} // namespace vk
} // namespace render
} // namespace engine
