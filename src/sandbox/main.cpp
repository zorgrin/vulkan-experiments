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


#include <QtWidgets/QApplication> //FIXME Gui

#include <QtGui/QWindow>

#include <engine/Engine.h>
#include <engine/SurfaceBackend.h>

#include "TestRenderScene.h"

class Application Q_DECL_FINAL: public QApplication
{
public:
    using QApplication::QApplication;

    bool notify(QObject *object, QEvent *event) Q_DECL_OVERRIDE
    {
        try
        {
            return QApplication::notify(object, event);
        }
        catch (std::exception &e)
        {
            qDebug() <<__FUNCTION__ << __LINE__ << e.what();
            QGuiApplication::quit();
            return false;
        }
        catch (...)
        {
            qFatal("unknown exception");
            abort(); // [[noreturn]]
            return false; //FIXME supress warning
        }
    }
};

int main(int argc, char **argv)
{
    try
    {
        Application app(argc, argv);

        QWindow w;
        w.show();
        w.resize(1024, 1024);
        std::unique_ptr<engine::render::vk::Engine> engine(new engine::render::vk::Engine(&w));
        engine->init();

        std::unique_ptr<engine::render::vk::test::TestRenderScene> debugScene(new engine::render::vk::test::TestRenderScene(engine->device(), &w));
        engine->installRenderHandler(debugScene.get());
        static_cast<engine::render::vk::SurfaceBackend*>(engine->frameSwapObject())->setSourceAttachment(debugScene->finalAttachment());

        int result;
        TRACE();
        result = app.exec();
        TRACE();
        engine->removeRenderHandler(debugScene.get());
        debugScene.reset();
        engine.reset();
        TRACE();
        qDebug() << "ended";
        return result;
    }
    catch (std::exception &e)
    {
        qCritical() << e.what();
        return 1;
    }
}


