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


#include "Test.h"
#include "TestRenderScene.h"

#include <engine/RenderObject.h>
#include <engine/Utils.h>
#include <engine/SimpleBufferObject.h>
#include <engine/ImageObject.h>
#include <engine/RenderBatch.h>
#include <engine/RenderObjectGroup.h>
#include <engine/hardcode.h>

#include <wrapper/Device.h>
#include <wrapper/Queue.h>
#include <wrapper/Image.h>
#include <wrapper/Fence.h>
#include <wrapper/CommandPool.h>
#include <wrapper/CommandBuffer.h>
#include <wrapper/exceptions.hpp>

#include <QtGui/QVector3D>
#include <QtGui/QVector2D>
#include <QtGui/QMatrix4x4>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

namespace engine
{
namespace render
{
namespace vk
{
namespace test
{

class TestFailed: public core::Exception
{
public:
    using Exception::Exception;

    ~TestFailed() Q_DECL_NOEXCEPT
    {

    }
};

void Test::readObj(CommandPool* pool, TestRenderScene* scene, std::unique_ptr<Model>& model, const std::string& obj, const std::string& diffuse, const QMatrix4x4& transform)
{
    auto device(pool->device());
    QFile file(QString::fromLocal8Bit(obj.c_str()));
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        std::vector<QVector3D> position;
        std::vector<QVector2D> texCoord;
        std::vector<QVector2D> pseudoTexCoord;
        std::vector<std::uint32_t> indices;
        std::map<std::uint32_t, std::uint32_t> pseudoTexIndices;

        while (!stream.atEnd())
        {
            QStringList data(stream.readLine().split(" "));
            if (data.at(0) == "v")
            {
                assert(data.size() == 4);
                position.push_back(QVector3D(data.at(1).toFloat(), data.at(2).toFloat(), data.at(3).toFloat()));
            }
            else if (data.at(0) == "vt")
            {
                assert(data.size() == 3);
                pseudoTexCoord.push_back(QVector2D(data.at(1).toFloat(), 1.0f - data.at(2).toFloat())); //FIXME is obj data inverted?
            }
            else if (data.at(0) == "f")
            {
                assert(data.size() == 4);
                auto indexLines(data.mid(1));
                for (auto i: indexLines)
                {
                    auto data(i.split("/"));
                    auto posIndex(data.at(0).toUInt() - 1);
                    indices.push_back(posIndex);
                    auto texIndex(data.at(1).toUInt() - 1);
                    assert(texIndex < pseudoTexCoord.size());
                    pseudoTexIndices[posIndex] = texIndex;
                }
            }
        }
        texCoord.resize(position.size());
        for (auto i: pseudoTexIndices)
        {
            assert(i.first < pseudoTexCoord.size());
            texCoord[i.first] = pseudoTexCoord.at(i.second);
        }

        for (auto i: indices)
            assert(i < position.size());
        assert(position.size() == texCoord.size());

        model.reset(new Model());
        RenderObject::InputData inputData;
        {
            {
                using Data = decltype(position);
                Data data(position);

                const std::size_t size(data.size() * sizeof(Data::value_type));
                model->position.reset(new SimpleBufferObject(device));
                model->position->reset(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                model->position->write(0, data.data(), size);
            }
            {
                using Data = decltype(texCoord);
                Data data(texCoord);

                const std::size_t size(data.size() * sizeof(Data::value_type));
                model->texCoord.reset(new SimpleBufferObject(device));
                model->texCoord->reset(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                model->texCoord->write(0, data.data(), size);
            }
            {
                using Data = decltype(indices);
                Data data(indices);

                inputData.indexCount = static_cast<std::uint32_t>(data.size());

                const std::size_t size(data.size() * sizeof(Data::value_type));
                model->index.reset(new SimpleBufferObject(device));
                model->index->reset(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                model->index->write(0, data.data(), size);
            }

            {
                using InstanceData = QGenericMatrix<4, 4, float>;
                using Data = std::vector<InstanceData>;
                Data data;
                data.resize(1000);
                const std::size_t size(data.size() * sizeof(Data::value_type));
                {
                    QMatrix4x4 t;
                    t.translate(30, 0, 0);
                    t *= transform;
                    data[0] = t.toGenericMatrix<4, 4>();
                }
                {
                    QMatrix4x4 t;
                    t.rotate(QQuaternion::fromEulerAngles(30, 30, 10));
                    t.translate(-30, 0, 0);
                    t *= transform;
                    data[1] = t.toGenericMatrix<4, 4>();
                }

                model->ssbo.reset(new SimpleBufferObject(device));
                model->ssbo->reset(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                model->ssbo->write(0, data.data(), size);
            }

            {
                model->diffuse.reset(new CombinedImageObject(device));
                model->diffuse->reset(
                    VK_IMAGE_TYPE_2D,
                    VK_FORMAT_B8G8R8A8_UNORM,
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_IMAGE_VIEW_TYPE_2D,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_SAMPLE_COUNT_1_BIT,
                    true
                );

                loadTexture(pool, model->diffuse.get(), diffuse);
            }
        }
        {
            inputData.attributes[0] = model->position.get();
            inputData.attributes[1] = model->texCoord.get();
            inputData.storages[0] = model->ssbo.get();
            inputData.images[1] = model->diffuse.get();
            inputData.indexBuffer = model->index.get();
        }

        model->object.reset(new RenderObject(device, scene->test_meshGroup()->objectLayout(), inputData));
        model->object->setInstanceCount(2);//FIXME test

        scene->test_addMeshPassObject(model->object.get());
    }
    else
    {
        throw TestFailed(file.errorString().toStdString() + ", " + obj);
    }
}

Test::Test(Device* device, TestRenderScene* scene)
{
    constexpr const char* ogreMesh("/assets/ogre/bs_rest.obj");
    constexpr const char* ogreDiffuse("/assets/ogre/diffuse.png");
    QMatrix4x4 ogreTransform;
    ogreTransform.scale(50.0f, 50.0f, -50.0f);
    ogreTransform.translate(1.0f, 1.8f);

    constexpr const char* spotMesh("/assets/spot/spot_triangulated.obj");
    constexpr const char* spotDiffuse("/assets/spot/spot_texture.png");
    QMatrix4x4 spotTransform;
    spotTransform.scale(50.0f, 50.0f, 50.0f);


    const std::string root(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../").toStdString());

    core::ReferenceObjectList<CommandPool>::ObjectRef commandPool(device->createCommandPool());

    readObj(commandPool.ptr(), scene, model0, root + ogreMesh, root + ogreDiffuse, ogreTransform);
    readObj(commandPool.ptr(), scene, model1, root + spotMesh, root + spotDiffuse, spotTransform);

}

Test::~Test()
{
    model0->object.reset();
    model0.reset();
    model1->object.reset();
    model1.reset();
}

static void testMultisampleClear(Device* device)
{
    std::unique_ptr<CombinedImageObject> colorImage(new CombinedImageObject(device));

    colorImage->reset(VK_IMAGE_TYPE_2D,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_SAMPLE_COUNT_4_BIT,
        true);

    colorImage->resize(VkExtent3D{512, 512, 1});

    auto commandPool(device->createCommandPool());
    auto clear(commandPool->createCommandBuffer());

    { // clear phase

        clear->reset();
        clear->enqueueCommand([&](CommandBuffer* buffer)
        {
            //TODO transition is required?
            {

                VkImageMemoryBarrier barrier;
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = nullptr;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //FIXME check
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                barrier.image = colorImage->image()->handle();

                vkCmdPipelineBarrier(buffer->handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            }

            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = VK_REMAINING_MIP_LEVELS;
            range.baseArrayLayer = 0;
            range.layerCount = VK_REMAINING_ARRAY_LAYERS;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

            VkClearColorValue clearColor{};

            vkCmdClearColorImage(buffer->handle(), colorImage->image()->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
        });
        clear->finalize();
    }
    VkSubmitInfo submitClear{};

    VkPipelineStageFlags flags(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    {
        submitClear.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitClear.pNext = nullptr;

        submitClear.waitSemaphoreCount = 0;
        submitClear.pWaitSemaphores = nullptr;

        submitClear.pWaitDstStageMask = &flags;

        submitClear.commandBufferCount = 1;
        submitClear.pCommandBuffers = clear->handlePtr();

        submitClear.signalSemaphoreCount = 0;
        submitClear.pSignalSemaphores = nullptr;
    }

    auto fence(device->createFence());
    device->defaultQueue().submit(submitClear, fence->handle());
    bool ok(fence->wait(hardcode::gFenceWait));

    fence.reset();
    clear.reset();
    commandPool.reset();
    colorImage.reset();

//    assert(ok);
    if (!ok)
        throw TestFailed(std::string("test: ") + __FUNCTION__ + ", reason: timeout");

    qDebug("[PASSED] %s", __FUNCTION__);
}

void runTests(Device* device)
{
    try
    {
        testMultisampleClear(device);
    }
    catch (std::exception& e)
    {
        qDebug("[FAILED] %s", e.what());
        throw;
    }
}

} // namespace test
} // namespace vk
} // namespace render
} // namespace engine
