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

#include <QtGui/QGenericMatrix>

#include <set>

namespace engine
{
namespace render
{
namespace vk
{

class Device;
class DescriptorPool;
class DescriptorSet;
class DescriptorSetLayout;
class RenderPass;
class PipelineLayout;
class Pipeline;

class SimpleBufferObject;
class CombinedImageObject;
class RenderObject;

class RenderObjectGroup
{
public:
    struct DescriptorSetFormat
    {
        using BindingPoint = unsigned;
        std::set<BindingPoint> uniforms;
        std::set<BindingPoint> storages;
        std::set<BindingPoint> combinedImages;
    };

    RenderObjectGroup(
        Device* device,
        DescriptorSetLayout* attachmentLayout,
        const DescriptorSetFormat& groupSetFormat,
        const DescriptorSetFormat& objectSetFormat);
    ~RenderObjectGroup();

    void addObject(RenderObject* object);

    PipelineLayout* pipelineLayout() const;

    DescriptorSetLayout* objectLayout() const;

    DescriptorSet* groupSet() const;

    std::list<RenderObject*> objects() const;

    SimpleBufferObject* uniform(unsigned bindingPoint) const;

    SimpleBufferObject* storage(unsigned bindingPoint) const;

    CombinedImageObject* combinedImage(unsigned bindingPoint) const;

    void finalize();

private:
    void init();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

} // namespace vk
} // namespace render
} // namespace engine
