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


#include "ShaderModule.h"
#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"

#include <fstream>

namespace engine
{
namespace render
{
namespace vk
{

struct ShaderModule::Private
{
    Device* device;
    
    Private(Device* device): device(device)
    {
    }
};

ShaderModule::ShaderModule(Device* device, const VkShaderModuleCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(device))
{
    validateStruct(m_info);

    auto r(vkCreateShaderModule(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

ShaderModule::~ShaderModule()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyShaderModule(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

Device* ShaderModule::device() const
{
    return m_d->device;
}

ShaderModule* ShaderModule::create(Device* device, const std::string &filename)
{
    std::ifstream stream;
    stream.open(filename, std::ios::in | std::ios::binary);
    if (stream.fail())
        throw core::Exception(std::string("unable to open shader ") + filename);

    std::vector<std::ifstream::char_type> buffer;
    {
        stream.seekg(0, std::ios_base::end);
        assert(!stream.fail());
        buffer.resize(static_cast<std::size_t>(stream.tellg()));
        stream.seekg(0, std::ios_base::beg);
        assert(!stream.fail());
        assert((buffer.size() % sizeof(std::uint32_t)) == 0);
    }
    stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    assert(!stream.fail());

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.codeSize = buffer.size() / sizeof(std::ifstream::char_type);
    info.pCode = reinterpret_cast<std::uint32_t*>(buffer.data());


    return new ShaderModule(device, info);
}

} // namespace vk
} // namespace render
} // namespace engine
