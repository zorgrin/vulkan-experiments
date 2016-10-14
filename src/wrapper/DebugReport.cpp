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


#include "DebugReport.h"

#include "Allocator.h"
#include "exceptions.hpp"
#include "Instance.h"

#include <set>

namespace engine
{
namespace render
{
namespace vk
{

namespace
{
    static VkBool32 debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        std::size_t location,
        int32_t messageCode,
        const char* layerPrefix,
        const char* message,
        void* userData)
    {
        return static_cast<DebugReport*>(userData)->handleDebug(flags, objectType, object, location, messageCode, layerPrefix, message);
    }
}

struct DebugReport::Private
{    
    Instance* instance;

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT = nullptr;
    
    std::set<IDebugDelegate*> handlers;

    bool isEnabled = true;

    static std::set<DebugReport*> debugSanityCheck; // FIXME validation bug workaround

    Private(Instance* instance): instance(instance)
    {
        CreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance->handle(), "vkCreateDebugReportCallbackEXT"));
        DestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance->handle(), "vkDestroyDebugReportCallbackEXT"));
        if ((CreateDebugReportCallbackEXT == nullptr) || (DestroyDebugReportCallbackEXT == nullptr))
            throw core::Exception("unable to load library symbols");
    }
};

std::set<DebugReport*> DebugReport::Private::debugSanityCheck;

DebugReport::DebugReport(Instance* instance): Handle(VK_NULL_HANDLE), CreationInfo(), m_d(new Private(instance))
{
    Private::debugSanityCheck.insert(this);

    m_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    m_info.pNext = nullptr;

    m_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    m_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
    m_info.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    m_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
    m_info.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;

    m_info.pfnCallback = debugCallback;
    m_info.pUserData = this;

    validateStruct(m_info);

    auto r(m_d->CreateDebugReportCallbackEXT(m_d->instance->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

DebugReport::~DebugReport()
{
    if (!m_d->handlers.empty())
        qWarning("Warning: debug message handlers are not uninstalled");

    if (m_handle != VK_NULL_HANDLE)
        m_d->DestroyDebugReportCallbackEXT(m_d->instance->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());

    Private::debugSanityCheck.erase(this);
}

void DebugReport::installHandler(IDebugDelegate* handler)
{
    m_d->handlers.insert(handler);
    //TODO sanity check
}

void DebugReport::uninstallHandler(IDebugDelegate* handler)
{
    m_d->handlers.erase(handler);
    //TODO sanity check
}

VkBool32 DebugReport::handleDebug(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    std::size_t location,
    int32_t messageCode,
    const char* layerPrefix,
    const char* message)
{
//    assert(Private::debugSanityCheck.find(this) != Private::debugSanityCheck.end());
    if (Private::debugSanityCheck.find(this) == Private::debugSanityCheck.end())
    {
        qDebug() << message;
        qCritical() << "Error: trying to access to destroyed object";
        return VK_FALSE;
    }
    if (m_d->isEnabled)
    {
        for (auto i: m_d->handlers)
            i->handleDebug(flags, objectType, object, location, messageCode, layerPrefix, message);
    }

    return VK_FALSE;
}

void DebugReport::setIsEnabled(bool enabled)
{
    m_d->isEnabled = enabled;
}

} // namespace vk
} // namespace render
} // namespace engine
