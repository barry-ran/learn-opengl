#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

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

    // 离屏渲染相关

    //顶点数组对象(Vertex Array Object，VAO)，用来缓存顶点缓冲对象的操作（例如setAttributeBuffer）
    QOpenGLVertexArrayObject m_offScreenVao;
    // 顶点缓冲对象(Vertex Buffer Objects, VBO)，用来缓存顶点坐标
    QOpenGLBuffer m_offScreenVbo; // 默认即为VertexBuffer(GL_ARRAY_BUFFER)类型
    // 索引缓冲对象(Element Buffer Object，EBO或Index Buffer Object，IBO)，用来缓存顶点坐标的索引
    QOpenGLBuffer m_offScreenEbo {QOpenGLBuffer::IndexBuffer};
    // 着色器程序：编译链接着色器
    QOpenGLShaderProgram m_offScreenShaderProgram;
    // 纹理
    QOpenGLTexture m_offScreenTexture {QOpenGLTexture::Target2D};
    QOpenGLTexture m_offScreenTexture2 {QOpenGLTexture::Target2D};
    // 离屏渲染到fbo的纹理附件中
    QOpenGLFramebufferObject* m_offScreenFbo = nullptr;
    QSize m_offScreenSize;


    // 屏幕渲染相关

    //顶点数组对象(Vertex Array Object，VAO)，用来缓存顶点缓冲对象的操作（例如setAttributeBuffer）
    QOpenGLVertexArrayObject m_screenVao;
    // 顶点缓冲对象(Vertex Buffer Objects, VBO)，用来缓存顶点坐标
    QOpenGLBuffer m_screenVbo; // 默认即为VertexBuffer(GL_ARRAY_BUFFER)类型
    // 索引缓冲对象(Element Buffer Object，EBO或Index Buffer Object，IBO)，用来缓存顶点坐标的索引
    QOpenGLBuffer m_screenEbo {QOpenGLBuffer::IndexBuffer};
    // 着色器程序：编译链接着色器
    QOpenGLShaderProgram m_screenShaderProgram;
};

#endif // WIDGET_H
