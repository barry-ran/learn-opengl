#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QImage>

/*
 * 使用纹理在矩形上显示图片
*/

float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
    0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
    -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
};

unsigned int indices[] = { // 注意索引从0开始!
                           0, 1, 3, // 第一个三角形
                           1, 2, 3  // 第二个三角形
                         };

const char* vertexShaderSource = R"(#version 330 core
                                 layout (location = 0) in vec3 aPos; // 来自cpu的顶点坐标
                                 layout (location = 1) in vec3 aColor; // 来自cpu的颜色数据
                                 layout (location = 2) in vec2 aTexCoord; // 来自cpu的纹理坐标

                                 // 颜色和纹理坐标用于输出到片段着色器
                                 out vec3 ourColor;
                                 out vec2 TexCoord;

                                 void main()
                                 {
                                 gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
                                 ourColor = aColor;
                                 TexCoord = aTexCoord;
                                 })";

const char* fragmentShaderSource = R"(#version 330 core
                                   out vec4 FragColor;

                                   in vec3 ourColor;
                                   in vec2 TexCoord;

                                   uniform sampler2D ourTexture;

                                   void main()
                                   {
                                   // 混合纹理坐标/纹理数据/颜色
                                   FragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0);
                                   } )";

Widget::Widget(QWidget *parent) :
    QOpenGLWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    makeCurrent();
    m_vbo.destroy();
    doneCurrent();

    delete ui;
}

void Widget::initializeGL()
{
    qDebug() << "format:" << context()->format();

    initializeOpenGLFunctions();
    // 黑色背景
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

    // vao初始化
    m_vao.create();
    // 绑定vao，下面对vbo/ebo的操作都保存到了vao上，下次绘制该物体直接m_vao.bind()即可，不用重复vbo/ebo的bind/allocate操作
    // vao也会保存ebo
    m_vao.bind();

    // vbo初始化
    m_vbo.create();
    m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vbo.allocate(vertices, sizeof(vertices));

    // ebo初始化
    m_ebo.create();
    m_ebo.bind();
    m_ebo.allocate(indices, sizeof(indices));

    // 编译着色器
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_shaderProgram.link();
    m_shaderProgram.bind();

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数location，顶点坐标为float，起始偏移为0字节，顶点坐标类型为vec3，步幅为3个float
    m_shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0 * sizeof(float), 3, 8 * sizeof(float));
    // 启用顶点属性
    m_shaderProgram.enableAttributeArray(0);

    m_shaderProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));
    m_shaderProgram.enableAttributeArray(1);

    m_shaderProgram.setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));
    m_shaderProgram.enableAttributeArray(2);

    // 设置纹理
    // 设置st方向上纹理超出坐标时的显示策略
    m_texture.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_texture.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // 设置纹理缩放时的策略
    m_texture.setMinificationFilter(QOpenGLTexture::Linear);
    m_texture.setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture.setData(QImage(":/wall.jpg"));

    // 图片是rgba8888格式，注意这里的参数GL_RGBA8/GL_RGBA
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, wall.width(), wall.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, wall.bits());

    m_vao.release();
    qDebug() << m_shaderProgram.log();
}

void Widget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void Widget::paintGL()
{
    // 绑定纹理
    m_texture.bind();

    // 我们这里只有一个shaderProgram和vao，其实没有必要重复bind和release
    // 这里只是为了教学，在多个shaderProgram的情况下，这是标准操作
    m_shaderProgram.bind();
    m_vao.bind();
    // glDrawElements会根据ebo中的6个索引去vbo中找顶点位置
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_shaderProgram.release();
}
