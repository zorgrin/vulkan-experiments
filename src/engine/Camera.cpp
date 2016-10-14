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

#include "Camera.h"
#include "SimpleBufferObject.h"

#include <QtGui/QVector4D>
#include <QtGui/QVector3D>
#include <QtGui/QGenericMatrix>
#include <QtGui/QMatrix4x4>

#include <wrapper/exceptions.hpp>

namespace engine
{
namespace render
{
namespace
{

#pragma pack(push, 1)
struct CameraData
{
    QVector4D eye; // 4-float align
#if !defined(Q_CC_MSVC) //FIXME MSVC cosntexpr static_assert
    static_assert(sizeof(eye) == (4 * 4));
#endif

    QVector4D target;
#if !defined(Q_CC_MSVC)
    static_assert(sizeof(target) == (4 * 4));
#endif

    QGenericMatrix<4, 4, float> projection;
#if !defined(Q_CC_MSVC)
    static_assert(sizeof(projection) == (4 * 4 * 4));
#endif

    QGenericMatrix<4, 4, float> transform;
#if !defined(Q_CC_MSVC)
    static_assert(sizeof(transform) == (4 * 4 * 4));
#endif
};
#pragma pack(pop)

}

struct Camera::Private
{
    vk::SimpleBufferObject* bufferObject;
    CameraData data;

    QVector3D up;

    Private(vk::SimpleBufferObject* bufferObject): bufferObject(bufferObject)
    {
        data.eye = QVector3D(0.0, 0.0, 1.0); // FIXME hardcode
        up = QVector3D(0.0, -1.0, 0.0);
    }
    ~Private()
    {

    }

    void upload(void* data, std::size_t size)
    {
        std::ptrdiff_t offset(reinterpret_cast<std::uint8_t*>(data) - reinterpret_cast<std::uint8_t*>(&this->data));
        if ((offset < 0) || ((static_cast<std::size_t>(offset) + size) > sizeof(CameraData)))
            throw core::Exception("internal error");

        bufferObject->write(offset, data, size);
    }


    void uploadProjection()
    {
        upload(&data.projection, sizeof(data.projection));
    }

    void uploadTransform()
    {
        upload(&data.transform, sizeof(data.transform));
    }

    void uploadEye()
    {
        upload(&data.eye, sizeof(data.eye));
    }

    void uploadTarget()
    {
        upload(&data.target, sizeof(data.target));
    }

    void updateTransform()
    {
        QMatrix4x4 result;
        result.lookAt(data.eye.toVector3D(), data.target.toVector3D(), up);
        QMatrix4x4 system;
        data.transform = (result * system).toGenericMatrix<4, 4>();
        uploadTransform();
    }
};

std::size_t Camera::dataSize()
{
    return sizeof(CameraData);
}

Camera::Camera(vk::SimpleBufferObject* bufferObject):
        m_d(new Private(bufferObject))
{
    m_d->uploadEye();
    m_d->uploadTarget();
    m_d->uploadProjection();
    m_d->updateTransform();
}

Camera::~Camera()
{

}

QVector3D Camera::eye() const
{
    return m_d->data.eye.toVector3D();
}

void Camera::setEye(const QVector3D &value)
{
    m_d->data.eye = value;
    m_d->uploadEye();
    m_d->updateTransform();
}


QVector3D Camera::target() const
{
    return m_d->data.target.toVector3D();
}

void Camera::setTarget(const QVector3D &value)
{
    m_d->data.target = value;
    m_d->uploadTarget();
    m_d->updateTransform();
}

QMatrix4x4 Camera::projection() const
{
    return QMatrix4x4(m_d->data.projection);
}

void Camera::setProjection(const QMatrix4x4 &value)
{
    m_d->data.projection = value.toGenericMatrix<4, 4>();
    m_d->uploadProjection();
}


} // namespace render
} // namespace engine
