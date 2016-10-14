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


#include "Surface.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Swapchain.h"

#if defined(Q_OS_LINUX)
#include <QtX11Extras/QX11Info>
#elif defined(Q_OS_WIN)
//dummy
#else
#error "OS support is not implemented"
#endif

namespace engine
{
namespace render
{
namespace vk
{

struct Surface::Private
{
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR = nullptr;

#if defined(Q_OS_LINUX)
    PFN_vkCreateXcbSurfaceKHR CreateXcbSurfaceKHR = nullptr;
#elif defined(Q_OS_WIN)
    PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR = nullptr;
#endif
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR = nullptr;

    Instance* instance;
    const PhysicalDevice* physicalDevice;
    QWindow* window;
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> supportedFormats;

    template<typename T>
    static inline void friendDeleter(T* object)
    {
        delete object;
    }
    
    Private(Instance* instance, const PhysicalDevice* physicalDevice, QWindow* window): 
        instance(instance), physicalDevice(physicalDevice), window(window), capabilities{}
    {
        GetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(instance->handle(), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        GetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(instance->handle(), "vkGetPhysicalDeviceSurfaceFormatsKHR"));
#if defined(Q_OS_LINUX)
        CreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(vkGetInstanceProcAddr(instance->handle(), "vkCreateXcbSurfaceKHR"));
#elif defined(Q_OS_WIN)
        CreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance->handle(), "vkCreateWin32SurfaceKHR"));
#endif
        DestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance->handle(), "vkDestroySurfaceKHR"));

        if ((GetPhysicalDeviceSurfaceCapabilitiesKHR == nullptr) ||
            (GetPhysicalDeviceSurfaceFormatsKHR == nullptr) ||
#if defined(Q_OS_LINUX)
            (CreateXcbSurfaceKHR == nullptr) ||
#elif defined(Q_OS_WIN)
            (CreateWin32SurfaceKHR == nullptr) ||
#endif
            (DestroySurfaceKHR == nullptr))
            throw core::Exception("unable to load library symbols");
    }
};

namespace
{

template <typename Backend, typename... Args>
VkSurfaceKHR createSurfaceBackend(Instance* instance, Args... args);

#if defined(Q_OS_LINUX)
template <>
VkSurfaceKHR createSurfaceBackend<VkXcbSurfaceCreateInfoKHR>(Instance* instance, xcb_connection_t* connection, xcb_window_t window, PFN_vkCreateXcbSurfaceKHR CreateXcbSurfaceKHR)
{
    VkXcbSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    info.connection = connection;
    info.window = window;

    VkSurfaceKHR handle(VK_NULL_HANDLE);
    validateStruct(info);

    auto r(CreateXcbSurfaceKHR(instance->handle(), &info, MemoryManager::instance().allocator()->callbackHandler(), &handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);

    return handle;
}
#elif defined(Q_OS_WIN)
template <>
VkSurfaceKHR createSurfaceBackend<VkWin32SurfaceCreateInfoKHR>(Instance* instance, HINSTANCE hinstance, HWND hwnd, PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR)
{
    VkWin32SurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = hinstance;
    info.hwnd = hwnd;

    VkSurfaceKHR handle(VK_NULL_HANDLE);
    //validateStruct(info); //TODO

    auto r(CreateWin32SurfaceKHR(instance->handle(), &info, MemoryManager::instance().allocator()->callbackHandler(), &handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);

    return handle;
}
#endif
}

Surface::Surface(Instance* instance, const PhysicalDevice* physicalDevice, QWindow* window):
    Handle(VK_NULL_HANDLE), 
    m_d(new Private(instance, physicalDevice, window))
{
#if defined(Q_OS_LINUX)
    if (QGuiApplication::platformName() == "xcb")
    {
        xcb_window_t id(static_cast<xcb_window_t>(window->winId()));
        m_handle = createSurfaceBackend<VkXcbSurfaceCreateInfoKHR>(instance, QX11Info::connection(), id, m_d->CreateXcbSurfaceKHR);
    }
    else
    {
        throw core::Exception("platform is not supported");
    }
#elif defined(Q_OS_WIN)
        HWND hwnd(reinterpret_cast<HWND>(window->winId()));
        HINSTANCE hinstance(::GetModuleHandle(NULL));
        m_handle = createSurfaceBackend<VkWin32SurfaceCreateInfoKHR>(instance, hinstance, hwnd, m_d->CreateWin32SurfaceKHR);
#else
        throw core::Exception("internal error");
#endif

    auto r(m_d->GetPhysicalDeviceSurfaceCapabilitiesKHR(m_d->physicalDevice->handle(), m_handle, &m_d->capabilities));
    CHECK_VK_RESULT(r, VK_SUCCESS);

    {
        std::uint32_t formatCount(0);
        auto r(m_d->GetPhysicalDeviceSurfaceFormatsKHR(m_d->physicalDevice->handle(), m_handle, &formatCount, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);

        m_d->supportedFormats.resize(formatCount);
        r = m_d->GetPhysicalDeviceSurfaceFormatsKHR(m_d->physicalDevice->handle(), m_handle, &formatCount, m_d->supportedFormats.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);
    }
}

Surface::~Surface()
{
    if (m_handle != VK_NULL_HANDLE)
        m_d->DestroySurfaceKHR(m_d->physicalDevice->instance()->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

const PhysicalDevice* Surface::physicalDevice() const
{
    return m_d->physicalDevice;
}

Instance* Surface::instance() const
{
    return m_d->instance;
}

VkSurfaceCapabilitiesKHR Surface::capabilities() const
{
    return m_d->capabilities;
}

VkExtent2D Surface::size() const
{
    return VkExtent2D{static_cast<std::uint32_t>(m_d->window->width()), static_cast<std::uint32_t>(m_d->window->height())};
}
std::vector<VkSurfaceFormatKHR> Surface::supportedFormats() const
{
    return m_d->supportedFormats;
}

} // namespace vk
} // namespace render
} // namespace engine
