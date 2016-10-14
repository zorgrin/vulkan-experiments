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

#include "CameraController.h"
#include "ICamera.h"

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <wrapper/exceptions.hpp>

namespace engine
{
namespace render
{

struct CameraController::Private
{
    ICamera *camera;

    enum Direction
    {
        None = 0,
        PanLeft = 1,
        PanRight = 2,
        PanUp = 4,
        PanDown = 8,

    };
    Q_DECLARE_FLAGS(Directions, Direction);
    Directions directions;

    Private(ICamera *camera): camera(camera)
    {

    }
};

CameraController::CameraController(ICamera *camera): m_d(new Private(camera))
{
}

CameraController::~CameraController()
{

}

ICamera* CameraController::camera() const
{
    return m_d->camera;
}

void CameraController::step(float elapsed)
{
    QVector3D delta;
    const float speed(1); //FIXME hardcode
    const float factor((m_d->camera->eye() - m_d->camera->target()).length());

    if (m_d->directions.testFlag(Private::PanLeft))
        delta = QVector3D(QVector3D(-speed, 0, 0) * elapsed);
    if (m_d->directions.testFlag(Private::PanRight))
        delta = QVector3D(QVector3D(+speed, 0, 0) * elapsed);
    if (m_d->directions.testFlag(Private::PanUp))
        delta = QVector3D(QVector3D(0, +speed, 0) * elapsed);
    if (m_d->directions.testFlag(Private::PanDown))
        delta = QVector3D(QVector3D(0, -speed, 0) * elapsed);

    m_d->camera->setEye(m_d->camera->eye() + delta * factor);
    m_d->camera->setTarget(m_d->camera->target() + delta * factor);
}

void CameraController::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::Key_Up:
        m_d->directions |= Private::PanUp;
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        m_d->directions |= Private::PanDown;
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        m_d->directions |= Private::PanLeft;
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_d->directions |= Private::PanRight;
        break;
    default:
        break; //ignore key
    }
}

void CameraController::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W:
    case Qt::Key_Up:
        m_d->directions &= ~Private::Directions(Private::PanUp);
        break;
    case Qt::Key_S:
    case Qt::Key_Down:
        m_d->directions &= ~Private::Directions(Private::PanDown);
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        m_d->directions &= ~Private::Directions(Private::PanLeft);
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_d->directions &= ~Private::Directions(Private::PanRight);
        break;
    default:
        break; //ignore key
    }
}

void CameraController::mousePressEvent(QMouseEvent *event)
{
    SOFT_NOT_IMPLEMENTED();
}

void CameraController::mouseMoveEvent(QMouseEvent *event)
{
//    SOFT_NOT_IMPLEMENTED();
}

void CameraController::mouseReleaseEvent(QMouseEvent *event)
{
    SOFT_NOT_IMPLEMENTED();
}

void CameraController::wheelEvent(QWheelEvent *event)
{
    QVector3D eye(m_d->camera->eye());
    const float zoomSpeed(4.0); //FIXME hardcode
    float delta((float)event->delta() / 15.0f / 8.0f / zoomSpeed);
    QVector3D direction(eye - m_d->camera->target());

    const float epsilon(1); //FIXME hardcode

    if (delta < 0 && direction.length() < epsilon)
        direction = direction.normalized() * epsilon;
    else if (delta > 0 && direction.length() < epsilon)
        direction = QVector3D();

    m_d->camera->setEye(eye - direction * delta);
}

} // namespace render
} // namespace engine
