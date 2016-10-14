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


#include "PhysicalDevice.h"

#include "Instance.h"
#include "Device.h"
#include "Surface.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

namespace hardcode //FIXME hardcode
{
    constexpr VkFormat gDefaultColorFormat(VK_FORMAT_B8G8R8A8_UNORM);
    constexpr VkFormat gDefaultDepthStencilFormat(VK_FORMAT_D32_SFLOAT_S8_UINT);
}


struct PhysicalDevice::Private
{
//    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR GetPhysicalDeviceDisplayPropertiesKHR = nullptr;
//    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR GetPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;

    
    Instance* instance;
    std::vector <VkQueueFamilyProperties> queueFamilies;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    std::vector <VkDisplayPropertiesKHR> displayProperties;
    std::vector <VkDisplayPlanePropertiesKHR> displayPlaneProperties;
    VkPhysicalDeviceFeatures features;

    core::ReferenceObjectList<Device> devices;
    VkFormat depthStencilFormat;
    VkFormat colorFormat;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(Instance* instance): 
        instance(instance), 
        properties{}, 
        memoryProperties{}, 
        devices(friendDeleter<Device>), 
        depthStencilFormat(hardcode::gDefaultDepthStencilFormat),
        colorFormat(hardcode::gDefaultColorFormat)
    {
//        GetPhysicalDeviceDisplayPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>(vkGetInstanceProcAddr(instance->handle(), "vkGetPhysicalDeviceDisplayPropertiesKHR"));
//        GetPhysicalDeviceDisplayPlanePropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR>(vkGetInstanceProcAddr(instance->handle(), "vkGetPhysicalDeviceDisplayPlanePropertiesKHR"));
//
//        if ((GetPhysicalDeviceDisplayPropertiesKHR == nullptr) || (GetPhysicalDeviceDisplayPlanePropertiesKHR == nullptr))
//            throw Exception("unable to load library symbols");
    }
};

PhysicalDevice::PhysicalDevice(Instance* instance, const VkPhysicalDevice &handle):
    Handle(handle), m_d(new Private(instance))
{
    {
        std::uint32_t count(0);
        vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &count, nullptr);
        m_d->queueFamilies.resize(count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &count, m_d->queueFamilies.data());

        vkGetPhysicalDeviceMemoryProperties(m_handle, &m_d->memoryProperties);
    }

    {
        vkGetPhysicalDeviceProperties(m_handle, &m_d->properties);
        qDebug() << QString("Device '%1' supports v%2.%3.%4 Vulkan API").arg(m_d->properties.deviceName).arg(VK_VERSION_MAJOR(m_d->properties.apiVersion)).arg(VK_VERSION_MINOR(m_d->properties.apiVersion)).arg(VK_VERSION_PATCH(m_d->properties.apiVersion));
        if (m_d->properties.apiVersion < instance->version())
            throw core::Exception("device does not support requested API version");
    }
    {
        vkGetPhysicalDeviceFeatures(m_handle, &m_d->features);
    }

    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(m_handle, m_d->depthStencilFormat, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
            throw core::Exception("no format found");

    }
    {
        for (unsigned i = 0; i < m_d->memoryProperties.memoryHeapCount; ++i)
        {
            // FIXME remove "" from output
            qDebug() << "memory heap" << QString("%1Mb%2").arg(m_d->memoryProperties.memoryHeaps[i].size / 1024 /1024).arg(((m_d->memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0) ? " (DEVICE_LOCAL)" : " (HOST)").toLocal8Bit().data();
            for (unsigned j = 0; j < m_d->memoryProperties.memoryTypeCount; ++j)
            {
                if (m_d->memoryProperties.memoryTypes[j].heapIndex == i)
                    qDebug() << "\tindex" << j << "flags" << QString("%1").arg(m_d->memoryProperties.memoryTypes[j].propertyFlags, 8, 2, QChar('0')).toLocal8Bit().data();
            }
        }
    }

//    {
//        std::uint32_t count(0);
//        auto r(m_d->GetPhysicalDeviceDisplayPropertiesKHR(m_handle, &count, nullptr));
//        CHECK_VK_RESULT(r, VK_SUCCESS);
//
//        m_d->displayProperties.resize(count);
//        r = m_d->GetPhysicalDeviceDisplayPropertiesKHR(m_handle, &count, m_d->displayProperties.data());
//        CHECK_VK_RESULT(r, VK_SUCCESS);
//        qDebug() << "enumerated" << count << "displays";
//        for (auto i: m_d->displayProperties)
//        {
//            qDebug() << "display" << i.displayName;
//        }
//    }
//    {
//        std::uint32_t count(0);
//        auto r(m_d->GetPhysicalDeviceDisplayPlanePropertiesKHR(m_handle, &count, nullptr));
//        CHECK_VK_RESULT(r, VK_SUCCESS);
//
//        m_d->displayPlaneProperties.resize(count);
//        r = m_d->GetPhysicalDeviceDisplayPlanePropertiesKHR(m_handle, &count, m_d->displayPlaneProperties.data());
//        CHECK_VK_RESULT(r, VK_SUCCESS);
//        qDebug() << "enumerated" << count << "planes";
//    }
    //TODO enumerate displays and modes
}

PhysicalDevice::~PhysicalDevice()
{
    m_d->devices.clear();
}

VkPhysicalDeviceProperties PhysicalDevice::properties() const
{
    return m_d->properties;
}

std::vector <VkQueueFamilyProperties> PhysicalDevice::queueFamilies() const
{
    return m_d->queueFamilies;
}

VkPhysicalDeviceFeatures PhysicalDevice::features() const
{
    return m_d->features;
}

core::ReferenceObjectList<Device>::ObjectRef PhysicalDevice::createDevice(unsigned family)
{
    return m_d->devices.append(new Device(this, family));
}

Instance* PhysicalDevice::instance() const
{
    return m_d->instance;
}

VkFormat PhysicalDevice::defaultDepthStencilFormat() const
{
    return m_d->depthStencilFormat;
}


VkFormat PhysicalDevice::defaultColorFormat() const
{
    return m_d->colorFormat;
}

unsigned PhysicalDevice::memoryFor(const VkMemoryRequirements& requirements, const VkMemoryPropertyFlags& flags) const
{
    for (unsigned i = 0; i < m_d->memoryProperties.memoryTypeCount; ++i)
        if ((requirements.memoryTypeBits & (1 << i)) != 0)
        {
            if ((m_d->memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        }

    throw core::Exception("no memory type found");
}


VkPhysicalDeviceMemoryProperties PhysicalDevice::memoryProperties() const
{
    return m_d->memoryProperties;
}


} // namespace vk
} // namespace render
} // namespace engine
