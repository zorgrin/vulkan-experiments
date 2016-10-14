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


#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSharedPointer>

#include <iostream>
#include <QDebug>

static QString format(const QString &file, const QString &input, QString &output)
{
    QStringList result;
    QStringList list(input.split('\n'));
    for (auto i: list)
    {
        if (i.startsWith("Warning, "))
        {
            QString line(i.mid(QString("Warning, ").length()));
            result << QString("%1: warning: %2").arg(file).arg(line);
        }
        else if (i.startsWith("ERROR: "))
        {
            QString line(i.mid(QString("ERROR: ").length()));
            QString dummy(line.left(line.indexOf(":")));
            line = line.mid(line.indexOf(":") + 1);
            QString lineNumber(line.left(line.indexOf(":")));
            line = line.mid(line.indexOf(":") + 1);
            result << QString("%1:%2:1: error: %3").arg(file).arg(lineNumber).arg(line);
        }
        else if (!i.trimmed().isEmpty())
        {
            output.append(file + ": " + i + "\n");
        }

    }
    return result.join("\n");
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (app.arguments().size() < 4)
    {
        qCritical() << "usage: shader-compiler <path-to-glslangValidator> <shared-shaders-dir> <output-dir>";
        return 1;
    }

    QString compiler(app.arguments().at(1));
    QString input(app.arguments().at(2));
    QString output(app.arguments().at(3));

    qDebug() << "Building SPIR-V binaries";
    qDebug() << compiler << input << output;
    QMap <QString, QString> prefixes;
    {
        prefixes["vs"] = "vert";
        prefixes["tes"] = "tese";
        prefixes["tcs"] = "tesc";
        prefixes["gs"] = "geom";
        prefixes["fs"] = "frag";
    }

    for (auto dir: QDir(input).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        QString shader(dir.baseName());
        qDebug() << "Compiling " << shader;

        for (auto f: QDir(dir.absoluteFilePath()).entryInfoList(QStringList() << "*.glsl", QDir::Files))
        {
            QString type(f.baseName());
            if (prefixes.contains(type))
            {
                QFile inputFile(f.absoluteFilePath());
                QByteArray buffer;
                if (inputFile.open(QIODevice::ReadOnly))
                {
                    buffer = inputFile.readAll();
                    inputFile.close();
                }
                else
                {
                    qCritical("error: %s", qPrintable(inputFile.errorString()));
                    return 1;
                }
                QFile outputTmpFile(QString(QString("%1%2shader-compiler-%3.%4").arg(QDir::tempPath()).arg(QDir::separator()).arg(shader).arg(prefixes.value(type))));
                if (!outputTmpFile.open(QIODevice::WriteOnly))
                {
                    qCritical("error: %s", qPrintable(outputTmpFile.errorString()));
                    return 1;
                }
//                qDebug() << "tmpfile" << files[type]->fileName();
                outputTmpFile.write(buffer); //TODO check
                outputTmpFile.close();

                {
                    QStringList args(QStringList() << outputTmpFile.fileName());
                    QString outputFile(QString("%1%2%3.%4.spirv").arg(output).arg(QDir::separator()).arg(shader).arg(type));
                    args << "-V" << "-o" << outputFile;

                    QProcess process;
                    process.start(compiler, args);
                    bool failed(false);
                    if (!process.waitForStarted())
                    {
                        qCritical("error: unable to launch %s, %s", qPrintable(compiler), qPrintable(process.errorString()));
                        failed = true;
                    }
                    if (!failed && !process.waitForFinished())
                    {
                        qCritical("error: unable finish %s, %s", qPrintable(compiler), qPrintable(process.errorString()));
                        failed = true;
                    }
                    QString stdoutput(process.readAllStandardOutput() + process.readAllStandardError());
                    QString log;
                    stdoutput = format(inputFile.fileName(), stdoutput, log);
                    if (!log.isEmpty())
                        std::cout << log.toStdString() << std::endl;
                    if (!stdoutput.isEmpty())
                        std::cerr << stdoutput.toStdString() << std::endl;

                    outputTmpFile.remove();

                    if (failed)
                    {
                        return 1;
                    }
                    if (!QFileInfo(outputFile).exists())
                    {
                        qCritical("error: output file haven't been created", qPrintable(outputFile));
                    }

                }
            }
            else
            {
                qCritical("error: unknown shader type %s for %s", qPrintable(type), qPrintable(shader));
                return 1;
            }
        }
    }

    return 0;
}


