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

class CommandPool;
class Device;
class DescriptorSetLayout;

class Renderer;
class SimpleBufferObject;
class CombinedImageObject;
class RenderObject;

namespace test
{

class TestRenderScene;

void runTests(Device* device);

class Test
{
public:
    Test(Device* device, TestRenderScene* scene);
    ~Test();

private:

//    std::unique_ptr<SimpleBufferObject> ubo;
    core::ReferenceObjectList<DescriptorSetLayout>::ObjectRef layout;

    struct Model
    {
        std::unique_ptr<SimpleBufferObject> ssbo;
        std::unique_ptr<SimpleBufferObject> position;
        std::unique_ptr<SimpleBufferObject> texCoord;

        std::unique_ptr<CombinedImageObject> diffuse;
        std::unique_ptr<SimpleBufferObject> index;

        std::unique_ptr<RenderObject> object;
    };
    std::unique_ptr<Model> model0;
    std::unique_ptr<Model> model1;

    void readObj(CommandPool* pool, TestRenderScene* scene, std::unique_ptr<Model>& model, const std::string& obj, const std::string& diffuse, const QMatrix4x4& transform);
};

} // namespace test
} // namespace vk
} // namespace render
} // namespace engine
