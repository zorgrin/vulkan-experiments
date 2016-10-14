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


#include "ImageView.h"

#include "Allocator.h"
#include "Device.h"
#include "exceptions.hpp"
#include "Image.h"

namespace engine
{
namespace render
{
namespace vk
{

struct ImageView::Private
{
    ImageBase* image;
    
    Private(ImageBase* image): image(image)
    {
    }
};

ImageView::ImageView(ImageBase* image, const VkImageViewCreateInfo &info): 
    Handle(VK_NULL_HANDLE), CreationInfo(info), m_d(new Private(image))
{
    validateStruct(m_info);

    assert(info.image == image->handle());
    auto r(vkCreateImageView(m_d->image->device()->handle(), &m_info, MemoryManager::instance().allocator()->callbackHandler(), &m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

ImageView::~ImageView()
{
    if (m_handle != VK_NULL_HANDLE)
        vkDestroyImageView(m_d->image->device()->handle(), m_handle, MemoryManager::instance().allocator()->callbackHandler());
}

ImageBase* ImageView::image() const
{
    return m_d->image;
}
//
//VkImageViewType ImageView::typeForImage(const VkImageType& type)
//{
//    switch (type)
//    {
//    case VK_IMAGE_TYPE_1D:
//        return VK_IMAGE_VIEW_TYPE_1D;
//    case VK_IMAGE_TYPE_2D:
//        return VK_IMAGE_VIEW_TYPE_2D;
//    case VK_IMAGE_TYPE_3D:
//        return VK_IMAGE_VIEW_TYPE_3D;
//    default:
//        throw Exception("invalid usage");
//    }
//}

} // namespace vk
} // namespace render
} // namespace engine
