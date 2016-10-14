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

#include <wrapper/defines.h>
#include <wrapper/reference-list.hpp>

namespace engine
{
namespace render
{
namespace vk
{

class Device;
class DescriptorSetLayout;
class DescriptorSet;
class RenderPass;
class Pipeline;

class CombinedImageObject;
class SimpleBufferObject;
class RenderObjectGroup;

class RenderBatch Q_DECL_FINAL
{
public:
    //TODO reduce input, inherit
    using AttachmentList = std::vector<CombinedImageObject*>;
    using SubpassIndex = unsigned;

    enum class ShaderStage //FIXME use vkcpp classes
    {
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    struct SubpassInputBinding
    {
        using BindingPoint = unsigned;
        using AttachmentReference = unsigned;

        struct Attachment
        {
            VkDescriptorSetLayoutBinding layout;
            AttachmentReference reference;
        };
        std::vector<Attachment> attachments;
    };

    RenderBatch(
        Device* device,
        RenderPass* renderPass,
        const AttachmentList& attachments,
        const std::vector<SubpassInputBinding>& subpasses);
    ~RenderBatch();

//    void addObject(RenderObject* object);
    void addGroup(SubpassIndex subpass, RenderObjectGroup* group, Pipeline* pipeline);

//    void updateData(const std::vector<SubpassData>& subpasses);

    RenderPass* renderPass() const;

    DescriptorSetLayout* attachmentLayout(SubpassIndex subpass) const;

    void render();

    void reset();
    void resize(unsigned width, unsigned height);

    void invalidate();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

} // namespace vk
} // namespace render
} // namespace engine
