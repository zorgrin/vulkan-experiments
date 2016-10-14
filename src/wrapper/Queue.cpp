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

#include "Queue.h"

#include "exceptions.hpp"

namespace engine
{
namespace render
{
namespace vk
{

Queue::Queue(const Queue& other): Handle(other.handle())
{

}

Queue::Queue(const VkQueue &handle): Handle(handle)
{

}

//void Queue::submit(const std::vector<VkSubmitInfo>& submits, const VkFence &fence)
//{
//    auto r(vkQueueSubmit(m_handle, submits.size(), submits.data(), fence));
//    CHECK_VK_RESULT(r, VK_SUCCESS);
//}

void Queue::submit(const VkSubmitInfo& submit, const VkFence &fence)
{
    validateStruct(submit);
    assert(submit.pWaitDstStageMask != nullptr);
    auto r(vkQueueSubmit(m_handle, 1, &submit, fence));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

void Queue::wait()
{
    auto r(vkQueueWaitIdle(m_handle));
    CHECK_VK_RESULT(r, VK_SUCCESS);
}

} // namespace vk
} // namespace render
} // namespace engine
