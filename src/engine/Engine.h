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

#include <wrapper/DebugReport.h>

#include <QtCore/QObject>

class QFile;

class QWindow;

namespace engine
{
namespace render
{

class IRenderable;

namespace vk
{

class Device;
class ISurfaceBackend;

class Engine Q_DECL_FINAL: public QObject, public IDebugDelegate
{
    Q_DISABLE_COPY(Engine)

public:
    Engine(QWindow* window);
    ~Engine();

    void init();

    void handleDebug(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        std::uint64_t object,
        std::size_t location,
        std::int32_t messageCode,
        const char* layerPrefix,
        const char* message) Q_DECL_OVERRIDE;

    Device* device() const;

    void installRenderHandler(IRenderable* renderHandler);
    void removeRenderHandler(IRenderable* renderHandler);

    ISurfaceBackend* frameSwapObject() const;

private:
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

    void updateGeometry();

    void paint();

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};

//void test(QWindow* window);

} // namespace vk
} // namespace render
} // namespace engine

