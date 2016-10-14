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

#include "Engine.h"
#include "SurfaceBackend.h"
#include "RenderObject.h"
#include "Utils.h"
#include "SimpleBufferObject.h"
#include "IRenderable.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtGui/QWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QMatrix4x4>

#include <wrapper/Device.h>
#include <wrapper/Instance.h>
#include <wrapper/PhysicalDevice.h>
#include <wrapper/PipelineCache.h>
#include <wrapper/CommandPool.h>
#include <wrapper/DescriptorPool.h>
#include <wrapper/exceptions.hpp>

#include "hardcode.h"

namespace engine
{
namespace render
{
namespace vk
{

struct Engine::Private
{
    QWindow* window;

    std::unique_ptr<Instance> instance;
    core::ReferenceObjectList<Device>::ObjectRef device;

    std::list<IRenderable*> renderers;
    std::unique_ptr<ISurfaceBackend> frameSwapObject;

    std::unique_ptr<QFile> logFile;

    std::unique_ptr<QTimer> timer;

    struct FPS
    {
        QDateTime last;
        std::list <float> values;
    } fps;

    Private(QWindow* window): window(window)
    {

    }
};

namespace
{

static QString toString(VkDebugReportObjectTypeEXT value)
{
    switch (value)
    {

    case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT: return "Instance";
    case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT: return "PhysicalDevice";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT: return "Device";
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT: return "Queue";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT: return "Semaphore";
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT: return "CommandBuffer";
    case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT: return "Fence";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT: return "DeviceMemory";
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: return "Buffer";
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: return "Image";
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT: return "Event";
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT: return "QueryPool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT: return "BufferView";
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT: return "ImageView";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT: return "ShaderModule";
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT: return "PipelineCache";
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT: return "PipelineLayout";
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT: return "RenderPass";
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: return "Pipeline";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT: return "DescriptorSetLayout";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT: return "Sampler";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT: return "DescriptorPool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: return "DescriptorSet";
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT: return "Framebuffer";
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT: return "CommandPool";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT: return "Surface";
    case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT: return "Swapchain";
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT: return "DebugReport";


    case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
    default: return "<UNKNOWN OBJECT>";
    }
}
}

Engine::Engine(QWindow* window):
    m_d(new Private(window))
{
    m_d->logFile.reset(new QFile("vulkan.log")); ///FIXME hardcoded
    m_d->logFile->open(QIODevice::WriteOnly); //TODO check
}

Engine::~Engine()
{
    m_d->window->removeEventFilter(this);

    m_d->frameSwapObject.reset();
    assert(m_d->renderers.empty());
//    m_d->renderers.clear();
    m_d->device.reset();

    m_d->instance->debugReporter()->uninstallHandler(this);

    m_d->logFile->close();
    m_d->logFile.reset();

    m_d->instance.reset();
}

void Engine::init()
{
    m_d->timer.reset(new QTimer());

    QObject::connect(m_d->timer.get(), &QTimer::timeout, [renderer = this]() -> void { renderer->paint(); });

    std::list<std::string> whitelistedLayers;
    if (true) //FIXME hardcode::gIsValidationEnabled
    {
        whitelistedLayers = std::list<std::string>
        {
            "VK_LAYER_LUNARG_standard_validation"
        };
    }

    m_d->instance.reset(new Instance("Engine test", VK_MAKE_VERSION(0, 0, 1), whitelistedLayers));
    m_d->instance->debugReporter()->installHandler(this);

    auto physicalDevice((*m_d->instance->physicalDevices().begin()));
    auto families(physicalDevice->queueFamilies());
    for (unsigned i = 0; i < families.size(); ++i)
    {
        if (families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_d->device = physicalDevice->createDevice(i);
            break;
        }
    }

    m_d->frameSwapObject.reset(new SurfaceBackend(m_d->device.ptr(), m_d->window));

    m_d->window->installEventFilter(this);

    qDebug() << "engine initialized";
}

void Engine::handleDebug(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    std::uint64_t object,
    std::size_t location,
    std::int32_t messageCode,
    const char* layerPrefix,
    const char* message)
{
    Q_UNUSED(location);
    Q_UNUSED(messageCode);

    QString prefix;
    switch (flags)
    {
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
        prefix = "[INFO]";
        break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        prefix = "[WARNING]";
        break;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        prefix = "[PERFORMANCE]";
        break;
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        prefix = "[ERROR]";
        break;
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        prefix = "[DEBUG]";
        break;
    default:
        assert("uknown flags" == 0);
        prefix = "[UNKNOWN]";
    }

    QString messageString(QString("%1 %2(0x%3) layerPrefix['%4']: '%5'").arg(prefix).arg(toString(objectType)).arg(object, 0, 16).arg(layerPrefix).arg(message));
//    qDebug("%s", qPrintable(messageString));


    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        m_d->logFile->write((messageString + "\n").toLocal8Bit());
        m_d->logFile->flush();

        qDebug() << messageString;
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        m_d->logFile->write((messageString + "\n").toLocal8Bit());
        m_d->logFile->flush();

        qDebug() << messageString;
    }
    else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        m_d->logFile->write((messageString + "\n").toLocal8Bit());
        m_d->logFile->flush();

        qDebug() << messageString;
    }
    else
    {
        //ignore API dump
    }
}

bool Engine::eventFilter(QObject* object, QEvent* event)
{
    assert(object == m_d->window);

    if (event->type() == QEvent::Expose)
    {
        m_d->timer->start(1000 / hardcode::gFPS);

        return false;
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_F11)
        {
            m_d->window->setVisibility(m_d->window->visibility() == QWindow::FullScreen ? QWindow::Windowed : QWindow::FullScreen);
        }

        return true;
    }
    else if (event->type() == QEvent::Resize)
    {
        updateGeometry();

        return false;
    }
    else if (event->type() == QEvent::Hide)
    {
        m_d->timer->stop();

        return false;
    }
    else if (event->type() == QEvent::WindowStateChange)
    {
        if (m_d->window->windowState() == Qt::WindowMinimized)
        {
            m_d->timer->stop();
            return false;
        }
    }


//    else
//    {
        return QObject::eventFilter(object, event);
//    }
}

void Engine::updateGeometry()
{
    QSize size(m_d->window->size());

    for (auto i: m_d->renderers)
        i->resize(static_cast<unsigned>(size.width()), static_cast<unsigned>(size.height()));

    m_d->frameSwapObject->updateGeometry();
}

void Engine::paint()
{
    {
        QDateTime now(QDateTime::currentDateTimeUtc());
        float elapsed(static_cast<float>(m_d->fps.last.msecsTo(now)) / 1000.0f);
        m_d->fps.last = now;
        m_d->fps.values.push_back(elapsed);

        while (m_d->fps.values.size() > 100)
            m_d->fps.values.pop_front();

        elapsed = std::accumulate(m_d->fps.values.begin(), m_d->fps.values.end(), 0.0f) / static_cast<float>(m_d->fps.values.size());

        m_d->window->setTitle(QString("FPS: %1").arg(1.0 / elapsed));

        for (auto i: m_d->renderers)
            i->render(elapsed);
    }

    m_d->frameSwapObject->swap();
}

Device* Engine::device() const
{
    return m_d->device.ptr();
}

void Engine::installRenderHandler(IRenderable* renderHandler)
{
    m_d->renderers.push_back(renderHandler);
}

void Engine::removeRenderHandler(IRenderable* renderHandler)
{
    m_d->renderers.remove(renderHandler);
}

ISurfaceBackend* Engine::frameSwapObject() const
{
    return m_d->frameSwapObject.get();
}

} // namespace vk
} // namespace render
} // namespace engine
