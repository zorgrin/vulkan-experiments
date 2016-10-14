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

#include <engine/IRenderable.h>
#include <engine/CameraController.h>

namespace engine
{
namespace render
{
class ICamera;

namespace vk
{

class Device;
class CommandBuffer;

class CombinedImageObject;
class RenderObjectGroup;
class RenderObject;

namespace test
{

class TestRenderScene Q_DECL_FINAL: public QObject, public IRenderable, public IInputHandler
{
public:
    TestRenderScene(Device* device, QObject* eventSource);
    ~TestRenderScene();

    [[deprecated]] void setup(CommandBuffer* buffer);

    CombinedImageObject* finalAttachment() const;

    RenderObjectGroup* test_meshGroup() const;

    void test_addMeshPassObject(RenderObject* object);

public:
    void resize(unsigned width, unsigned height) Q_DECL_OVERRIDE;

    void render(float elapsed) Q_DECL_OVERRIDE;

public:
    void step(float elapsed) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

protected:
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

} // namespace test
} // namespace vk
} // namespace render
} // namespace engine
