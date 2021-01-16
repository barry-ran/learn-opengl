#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

namespace Ui {
class Widget;
}

class Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

private:
    Ui::Widget *ui;

    //顶点数组对象(Vertex Array Object，VAO)，用来缓存顶点缓冲对象的操作（例如setAttributeBuffer）
    QOpenGLVertexArrayObject m_vao;
    // 顶点缓冲对象(Vertex Buffer Objects, VBO)，用来缓存顶点坐标
    QOpenGLBuffer m_vbo; // 默认即为VertexBuffer(GL_ARRAY_BUFFER)类型
    // 索引缓冲对象(Element Buffer Object，EBO或Index Buffer Object，IBO)，用来缓存顶点坐标的索引
    QOpenGLBuffer m_ebo {QOpenGLBuffer::IndexBuffer};

    // 着色器程序：编译链接着色器
    QOpenGLShaderProgram m_shaderProgram;

    // 纹理
    QOpenGLTexture m_texture {QOpenGLTexture::Target2D};
};

#endif // WIDGET_H
