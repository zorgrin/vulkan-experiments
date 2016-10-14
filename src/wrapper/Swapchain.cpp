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


#include "Swapchain.h"

#include "Instance.h"
#include "Device.h"
#include "Image.h"
#include "PhysicalDevice.h"
#include "Surface.h"
#include "Allocator.h"
#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

struct Swapchain::Private
{
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR = nullptr;

    PFN_vkCreateSwapchainKHR CreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR DestroySwapchainKHR = nullptr;
    PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
    PFN_vkAcquireNextImageKHR AcquireNextImageKHR = nullptr;

    Device* device;
    const Surface* surface;
    VkSurfaceCapabilitiesKHR surfaceCaps;
    std::vector<VkImage> images;
    
    Private(Device* device, const Surface* surface): device(device), surface(surface), surfaceCaps{}
    {
        GetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkGetPhysicalDeviceSurfacePresentModesKHR"));
        GetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkGetPhysicalDeviceSurfaceSupportKHR"));

        CreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkCreateSwapchainKHR"));
        DestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkDestroySwapchainKHR"));
        GetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkGetSwapchainImagesKHR"));
        AcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetInstanceProcAddr(device->physicalDevice()->instance()->handle(), "vkAcquireNextImageKHR"));

        if ((GetPhysicalDeviceSurfacePresentModesKHR == nullptr) ||
            (GetPhysicalDeviceSurfaceSupportKHR == nullptr) ||
            (CreateSwapchainKHR == nullptr) ||
            (DestroySwapchainKHR == nullptr) ||
            (GetSwapchainImagesKHR == nullptr) ||
            (AcquireNextImageKHR == nullptr))
            throw core::Exception("unable to load library symbols");

    }
};

Swapchain::Swapchain(Device* device, const Surface* surface, unsigned chainSize):
    Handle(VK_NULL_HANDLE),
    CreationInfo(), 
    m_d(new Private(device, surface))
{
    {
        VkBool32 result(VK_FALSE);
        auto r(m_d->GetPhysicalDeviceSurfaceSupportKHR(m_d->device->physicalDevice()->handle(), m_d->device->queueFamilyIndex(), m_d->surface->handle(), &result));
        CHECK_VK_RESULT(r, VK_SUCCESS);
        if (result != VK_TRUE)
            throw core::Exception("unsupported queue family");
    }

    m_d->surfaceCaps = m_d->surface->capabilities();
    {
        std::uint32_t totalCount(std::max(m_d->surfaceCaps.minImageCount, chainSize));
        if (m_d->surfaceCaps.maxImageCount > 0)
            totalCount = std::min(m_d->surfaceCaps.maxImageCount, totalCount);
        m_d->images.resize(totalCount);
    }

    m_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    m_info.pNext = nullptr;
    m_info.surface = m_d->surface->handle();
    m_info.minImageCount = static_cast<std::uint32_t>(m_d->images.size());

    VkSurfaceFormatKHR format;
    {
        std::vector<VkSurfaceFormatKHR> formats(m_d->surface->supportedFormats());

        bool valid(false);
        for (auto i: formats)
        {
            if (i.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                format = i;
                valid = true;
                break;
            }
        }
        if (!valid)
            throw core::Exception("no bgra8888 format present");

    }
    m_info.imageFormat = format.format;
    m_info.imageColorSpace = format.colorSpace;
    m_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    m_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    m_info.imageArrayLayers = 1;
    m_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_info.queueFamilyIndexCount = 0;
    m_info.pQueueFamilyIndices = nullptr;

    {
        std::uint32_t count(0);

        auto r(m_d->GetPhysicalDeviceSurfacePresentModesKHR(m_d->surface->physicalDevice()->handle(), m_d->surface->handle(), &count, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);

        std::vector<VkPresentModeKHR> modes(count);
        r = m_d->GetPhysicalDeviceSurfacePresentModesKHR(m_d->surface->physicalDevice()->handle(), m_d->surface->handle(), &count, modes.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);

//        for (auto i: modes)
//            qDebug() << "presentMode supported" << i;
        auto desiredMode(std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR));
        if (desiredMode != modes.end())
            m_info.presentMode = *desiredMode;
        else
            m_info.presentMode = *modes.begin();
    }
    m_info.clipped = true;
    m_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    reset();
}


void Swapchain::reset()
{
    const VkExtent2D minSize(m_d->surface->size());
    m_info.imageExtent.width = std::max<unsigned>(m_d->surfaceCaps.minImageExtent.width, minSize.width);
    m_info.imageExtent.height = std::max<unsigned>(m_d->surfaceCaps.minImageExtent.height, minSize.height);

#if 0 //TODO replace
    m_info.oldSwapchain = m_handle;
    validateStruct(m_info);

    VkSwapchainKHR previousHandle(m_handle);
    auto r(m_d->CreateSwapchainKHR(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));

    if (previousHandle != VK_NULL_HANDLE)
        m_d->DestroySwapchainKHR(m_d->device->handle(), previousHandle, MemoryManager::instance().allocator()->callbackHandler());

    CHECK_VK_RESULT(r, VK_SUCCESS);
#else
    m_info.oldSwapchain = VK_NULL_HANDLE;
    validateStruct(m_info);

    if (m_handle != VK_NULL_HANDLE)
    {
        m_d->DestroySwapchainKHR(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
        m_handle = VK_NULL_HANDLE;
    }

    assert(m_handle == nullptr);
    auto r(m_d->CreateSwapchainKHR(m_d->device->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);

#endif

    {
        std::uint32_t imagesCount(0);
        auto r(m_d->GetSwapchainImagesKHR(m_d->device->handle(), m_handle, &imagesCount, nullptr));
        CHECK_VK_RESULT(r, VK_SUCCESS);
        assert(imagesCount == m_d->images.size());
        m_d->images.resize(imagesCount);

        r = m_d->GetSwapchainImagesKHR(m_d->device->handle(), m_handle, &imagesCount, m_d->images.data());
        CHECK_VK_RESULT(r, VK_SUCCESS);
    }

}

Swapchain::~Swapchain()
{
    if (m_handle != VK_NULL_HANDLE)
        m_d->DestroySwapchainKHR(m_d->device->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

const Surface* Swapchain::surface() const
{
    return m_d->surface;
}

Device* Swapchain::device() const
{
    return m_d->device;
}

unsigned Swapchain::size() const
{
    return static_cast<unsigned>(m_d->images.size());
}

std::vector<VkImage> Swapchain::images() const
{
    return m_d->images;
}

VkResult Swapchain::acquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* imageIndex)
{
    return m_d->AcquireNextImageKHR(m_d->device->handle(), m_handle, timeout, semaphore, fence, imageIndex);
}

} // namespace vk
} // namespace render
} // namespace engine
