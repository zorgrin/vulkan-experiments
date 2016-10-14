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

namespace engine
{
namespace render
{
namespace vk
{

class Instance;

class IDebugDelegate
{
public:
    virtual ~IDebugDelegate() {}

    virtual void handleDebug(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        std::size_t location,
        int32_t messageCode,
        const char* layerPrefix,
        const char* message) = 0;

};

class DebugReport: public Handle<VkDebugReportCallbackEXT>, public CreationInfo<VkDebugReportCallbackCreateInfoEXT>
{
    Q_DISABLE_COPY(DebugReport)
public:
    explicit DebugReport(Instance* instance);
    virtual ~DebugReport();

    VkBool32 handleDebug(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        std::size_t location,
        int32_t messageCode,
        const char* layerPrefix,
        const char* message);

    void installHandler(IDebugDelegate* handler);
    void uninstallHandler(IDebugDelegate* handler);

    void setIsEnabled(bool enabled);

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};


} // namespace vk
} // namespace render
} // namespace engine
