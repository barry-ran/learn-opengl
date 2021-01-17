#include "widget.h"

#include <QApplication>
#include <QDebug>
#include <QtConcurrent>

#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

/*
 * 不使用ui surface的情况下进行QOpenGLFramebufferObject离屏渲染：
 * 之前虽然我们使用QOpenGLFramebufferObject实现了离屏渲染（渲染到了纹理）
 * 但是还是在窗口surface的环境下，这次我们使用QOffscreenSurface创建了离屏surface
 * 基于离屏surface环境进行渲染
 *
 * surface是可以渲染的设备：
 * 窗口surface是渲染到窗口
 * 离屏surface是渲染到图片/pbuffer等
*/

QImage processImage(const QImage& image,
                    const QString& vertexShader,
                    const QString& fragmentShader,
                    const QString& textureVar,
                    const QString& vertexPosVar,
                    const QString& textureCoordVar) {
    QOpenGLContext context;
    if(!context.create())
    {
        qDebug() << "Can't create GL context.";
        return {};
    }

    // 创建离屏surface，作为后续的渲染设备
    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if(!surface.isValid())
    {
        qDebug() << "Surface not valid.";
        return {};
    }

    // 将离屏surface关联到opengl上下文
    if(!context.makeCurrent(&surface))
    {
        qDebug() << "Can't make context current.";
        return {};
    }

    QOpenGLFramebufferObject fbo(image.size());
    context.functions()->glViewport(0, 0, image.width(), image.height());

    QOpenGLShaderProgram program(&context);
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader))
    {
        qDebug() << "Can't add vertex shader.";
        return {};
    }
    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader))
    {
        qDebug() << "Can't add fragment shader.";
        return {};
    }
    if (!program.link())
    {
        qDebug() << "Can't link program.";
        return {};
    }
    if (!program.bind())
    {
        qDebug() << "Can't bind program.";
        return {};
    }

    QOpenGLTexture texture(QOpenGLTexture::Target2D);
    texture.setData(image);

    texture.bind();
    if(!texture.isBound())
    {
        qDebug() << "Texture not bound.";
        return {};
    }

    struct VertexData
    {
        QVector2D position;
        QVector2D texCoord;
    };

    VertexData vertices[] =
    {
        {{ -1.0f, +1.0f }, { 0.0f, 1.0f }}, // top-left
        {{ +1.0f, +1.0f }, { 1.0f, 1.0f }}, // top-right
        {{ -1.0f, -1.0f }, { 0.0f, 0.0f }}, // bottom-left
        {{ +1.0f, -1.0f }, { 1.0f, 0.0f }}  // bottom-right
    };

    GLuint indices[] =
    {
        0, 1, 2, 3
    };

    QOpenGLBuffer vertexBuf(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer indexBuf(QOpenGLBuffer::IndexBuffer);

    if(!vertexBuf.create())
    {
        qDebug() << "Can't create vertex buffer.";
        return {};
    }

    if(!indexBuf.create())
    {
        qDebug() << "Can't create index buffer.";
        return {};
    }

    if(!vertexBuf.bind())
    {
        qDebug() << "Can't bind vertex buffer.";
        return {};
    }
    vertexBuf.allocate(vertices, 4 * sizeof(VertexData));

    if(!indexBuf.bind())
    {
        qDebug() << "Can't bind index buffer.";
        return {};
    }
    indexBuf.allocate(indices, 4 * sizeof(GLuint));

    int offset = 0;
    program.enableAttributeArray(vertexPosVar.toLatin1().data());
    program.setAttributeBuffer(vertexPosVar.toLatin1().data(), GL_FLOAT, offset, 2, sizeof(VertexData));
    offset += sizeof(QVector2D);
    program.enableAttributeArray(textureCoordVar.toLatin1().data());
    program.setAttributeBuffer(textureCoordVar.toLatin1().data(), GL_FLOAT, offset, 2, sizeof(VertexData));
    program.setUniformValue(textureVar.toLatin1().data(), 0);

    context.functions()->glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, Q_NULLPTR);

    // readpix读取渲染的数据
    return fbo.toImage(false);
}

void run() {
    qDebug() << "run thread id:" << (int64_t)QThread::currentThreadId();
    QString vertexShader =
            "attribute vec4 aPosition;\n"
            "attribute vec2 aTexCoord;\n"
            "varying vec2 vTexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = aPosition;\n"
            "   vTexCoord = aTexCoord;\n"
            "}";

    // 使用片段着色器将棕褐色效果应用于图像
    QString fragmentShader =
            "uniform sampler2D texture;\n"
            "varying vec2 vTexCoord;\n"
            "void main()\n"
            "{\n"
            "   vec2 uv = vTexCoord;\n"
            "   vec4 orig = texture2D(texture, uv);\n"
            "   vec3 col = orig.rgb;\n"
            "   float y = 0.3 * col.r + 0.59 * col.g + 0.11 * col.b;\n"
            "   gl_FragColor = vec4(y + 0.15, y + 0.07, y - 0.12, 1.0);\n"
            "}";


    QImage image = processImage(QImage(":/girls.jpeg"),
                                vertexShader,
                                fragmentShader,
                                "texture",
                                "aPosition",
                                "aTexCoord");

    image.save(QCoreApplication::applicationDirPath() + "/../../../out.jpeg");
}

int main(int argc, char *argv[])
{
    // 即使不需要窗口surface，但是opengl context要求必须是guiapplication程序
    QApplication a(argc, argv);
    /*
    QSurfaceFormat format;
    format.setVersion(3, 3);
    QSurfaceFormat::setDefaultFormat(format);
    */

    //Widget w;
    //w.show();

    qDebug() << "main thread id:" << (int64_t)QThread::currentThreadId();

    // 测试放在子线程中离屏渲染
     QFuture<void> future = QtConcurrent::run(run);
     future.waitForFinished();

    qDebug() << "processImage finish";

    //return a.exec();
}
