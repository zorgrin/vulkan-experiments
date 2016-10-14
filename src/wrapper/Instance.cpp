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


#include "Instance.h"

#include "Allocator.h"
#include "DebugReport.h"
#include "exceptions.hpp"
#include "PhysicalDevice.h"
#include "Surface.h"

#include <set>
#include <algorithm>

namespace engine
{
namespace render
{
namespace vk
{

namespace
{
    constexpr const char* gEngineID("Generic Vulkan engine");
    constexpr std::uint32_t gEngineVersion(VK_API_VERSION_1_0);
}

struct Instance::Private
{
    VkApplicationInfo applicationInfo;
    std::unique_ptr<DebugReport> debugReporter;
    core::ReferenceObjectList<PhysicalDevice> physicalDevices;
    std::vector<core::ReferenceObjectList<PhysicalDevice>::ObjectRef> physicalDeviceRefs;
    core::ReferenceObjectList<Surface> surfaces;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private():
        applicationInfo{}, 
        physicalDevices(friendDeleter<PhysicalDevice>), 
        surfaces(friendDeleter<Surface>)
    {
    }
};

Instance::Instance(
    const std::string& applicationName, 
    std::uint32_t applicationVersion, 
    const std::list<std::string>& requestedLayers): 
        Handle(VK_NULL_HANDLE), 
        CreationInfo(),
        m_d(new Private())

{
    m_d->applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_d->applicationInfo.pNext = nullptr;
    m_d->applicationInfo.pApplicationName = applicationName.c_str();
    m_d->applicationInfo.applicationVersion = applicationVersion;
    m_d->applicationInfo.pEngineName = gEngineID;
    m_d->applicationInfo.engineVersion = gEngineVersion;
    m_d->applicationInfo.apiVersion = 0;

    m_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    m_info.pNext = nullptr;
    m_info.flags = 0;
    m_info.pApplicationInfo = &m_d->applicationInfo;

    std::vector<std::string> layerNames;
    std::vector<const char*> layerPtrs;


    {
        std::uint32_t propertyCount(0);
        auto r(vkEnumerateInstanceLayerProperties(&propertyCount, nullptr));
        if (r == VK_INCOMPLETE)
            qDebug() << "incomplete layer properties";
        else if (r != VK_SUCCESS)
            throw Error(r);

        std::vector<VkLayerProperties> properties(propertyCount);
        std::set<std::string> available;
        r = vkEnumerateInstanceLayerProperties(&propertyCount, properties.data());
        if (r == VK_INCOMPLETE)
            qDebug() << "incomplete layer properties";
        else if (r != VK_SUCCESS)
            throw Error(r);
        for (auto i: properties)
        {
            available.insert(std::string(i.layerName));
            if (std::find(requestedLayers.begin(), requestedLayers.end(), std::string(i.layerName)) == requestedLayers.end()) //ignore
            {
                qDebug() <<"INSTANCE IGNORE"<< i.layerName << i.description;
            }
            else
            {
                layerNames.push_back(i.layerName);
                layerPtrs.push_back(layerNames.back().c_str());
            }
        }
        {
            std::set<std::string> unavailable;
            std::set<std::string> requested(requestedLayers.begin(), requestedLayers.end());
            std::set_difference(available.begin(), available.end(), requested.begin(), requested.end(), std::inserter(unavailable, unavailable.end()));
//            for (auto i: unavailable)
//                qDebug() << "INSTANCE UNKNOWN LAYERS" << i.c_str();
            unavailable.clear();
            std::set_difference(requested.begin(), requested.end(), available.begin(), available.end(), std::inserter(unavailable, unavailable.end()));
            for (auto i: unavailable)
                qDebug() << "INSTANCE UNSUPPORTED LAYERS" << i.c_str();
        }

    }

    m_info.enabledLayerCount = static_cast<std::uint32_t>(layerPtrs.size());
    m_info.ppEnabledLayerNames = layerPtrs.data();

    std::vector<VkExtensionProperties> supportedExtensions;
    {
        std::uint32_t count(0);
        auto r(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);

        supportedExtensions.resize(count);
        r = vkEnumerateInstanceExtensionProperties(nullptr, &count, supportedExtensions.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);
        if (supportedExtensions.empty())
            qDebug() << "no extensions";
        for (auto j: supportedExtensions)
            qDebug() << "supported" << j.extensionName << "ver." << j.specVersion;
    }

    std::vector <const char*> required;
    required.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(Q_OS_LINUX)
    required.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(Q_OS_WIN)
    required.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
//    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
    required.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
//    required.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);

    {
        std::set <std::string> requiredSet(required.begin(), required.end());

        std::set <std::string> supported;
        for (auto i: supportedExtensions)
            supported.insert(i.extensionName);

        std::set<std::string> unsupported;

        std::set_difference(
            requiredSet.begin(), requiredSet.end(),
            supported.begin(), supported.end(),
            std::inserter(unsupported, unsupported.end()));

        for (auto i: unsupported)
            qCritical("[FATAL] extension %s is not supported", i.c_str());

        if (!unsupported.empty())
            throw core::Exception("critical extension is not supported");
    }

    m_info.enabledExtensionCount = static_cast<std::uint32_t>(required.size());
    m_info.ppEnabledExtensionNames = required.data();

    validateStruct(m_info);

    auto result(vkCreateInstance(&m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    if (result != VK_SUCCESS)
        throw Error(result);

    m_d->debugReporter.reset(new DebugReport(this));

    {
        std::uint32_t count(0);

        auto r(vkEnumeratePhysicalDevices(m_handle, &count, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);

        std::vector<VkPhysicalDevice> devices;
        devices.resize(count);
        r = vkEnumeratePhysicalDevices(m_handle, &count, devices.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);

        for (auto i: devices)
        {
            auto ref(m_d->physicalDevices.append(new PhysicalDevice(this, i)));
            m_d->physicalDeviceRefs.push_back(ref);
        }

        qDebug() << "enumerated" << count << "devices";
    }
}

Instance::~Instance()
{
    m_d->surfaces.clear();
    m_d->physicalDeviceRefs.clear();
    m_d->physicalDevices.clear();
    m_d->debugReporter.reset();

    if (m_handle != VK_NULL_HANDLE)
        vkDestroyInstance(m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

std::vector<core::ReferenceObjectList<PhysicalDevice>::ObjectRef> Instance::physicalDevices() const
{
    return m_d->physicalDeviceRefs;
}

core::ReferenceObjectList<Surface>::ObjectRef Instance::createSurface(QWindow* window, const PhysicalDevice* device)
{
    return m_d->surfaces.append(new Surface(this, device, window));
}

std::uint32_t Instance::version() const
{
    return m_d->applicationInfo.apiVersion;
}

DebugReport* Instance::debugReporter() const
{
    return m_d->debugReporter.get();
}

} // namespace vk
} // namespace render
} // namespace engine
