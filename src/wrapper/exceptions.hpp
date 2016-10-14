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

#include <stdexcept>
#include "../wrapper/defines.h"

#include <chrono> //FIXME
#include <iostream>

namespace engine
{

namespace core //FIXME move to debug
{

template <typename Operation>
inline std::chrono::microseconds::rep estimateExecutionTime(bool print, const std::string &id, Operation op)
{
    auto pre(std::chrono::system_clock::now());
    op();
    auto post(std::chrono::system_clock::now());
    std::chrono::microseconds::rep elapsed(std::chrono::duration_cast<std::chrono::microseconds>(post - pre).count());
    if (print)
        std::cout << "BENCHMARK: " << id << " executed in " << elapsed << " us" << std::endl;

    return elapsed;
}

}

namespace core
{

class Exception: public std::runtime_error
{
public:
    using runtime_error::runtime_error;

    ~Exception() Q_DECL_NOEXCEPT
    {

    }
};

} // namespace core

namespace render
{
namespace vk
{

class Error: public core::Exception
{
public:
    Error(VkResult value): Exception(toString(value))
    {

    }

    ~Error() Q_DECL_NOEXCEPT
    {

    }

    static std::string toString(VkResult value)
    {
        switch (value)
        {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";

#if defined(Q_CC_GNU) || defined(Q_CC_CLANG)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#endif
        case VK_ERROR_INVALID_PARAMETER_NV: return "VK_ERROR_INVALID_PARAMETER_NV";
        case VK_ERROR_INVALID_ALIGNMENT_NV: return "VK_ERROR_INVALID_ALIGNMENT_NV";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
#if defined(Q_CC_GNU) || defined(Q_CC_CLANG)
#    pragma GCC diagnostic pop
#endif
        default: return "UNKNOWN(" + std::to_string(value) + ")";
        }
    }
};


} // namespace vk
} // namespace render
} // namespace engine

#define CHECK_VK_RESULT(result, expected) { if (result != expected) { qDebug() << "VkResult check failed" << __PRETTY_FUNCTION__ << __LINE__ << ::engine::render::vk::Error::toString(result).c_str(); throw ::engine::render::vk::Error(result); } }


