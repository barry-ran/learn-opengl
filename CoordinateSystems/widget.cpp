#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QImage>
#include <QTimer>
/*
 * 通过模型矩阵+视图矩阵+裁剪矩阵实现3d效果
 * opengl中坐标系统原理参考这里 https://learnopengl-cn.github.io/01%20Getting%20started/08%20Coordinate%20Systems/
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
                                 // 接收来自cpu的变换矩阵数据，用来移动/缩放
                                 uniform mat4 model;
                                 uniform mat4 view;
                                 uniform mat4 projection;

                                 // 颜色和纹理坐标用于输出到片段着色器
                                 out vec3 ourColor;
                                 out vec2 TexCoord;

                                 void main()
                                 {
                                 // 从右往左:局部坐标->世界坐标->观察坐标->裁剪坐标
                                 gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
                                 ourColor = aColor;
                                 // 片段着色器采样图片是从下向上采样，所以图像是倒的，这里反转一个y
                                 TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
                                 })";

const char* fragmentShaderSource = R"(#version 330 core
                                   out vec4 FragColor;

                                   in vec3 ourColor;
                                   in vec2 TexCoord;

                                   uniform sampler2D texture1;
                                   uniform sampler2D texture2;

                                   void main()
                                   {
                                   // 混合纹理坐标/纹理数据/颜色
                                   FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
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

    // 关联片段着色器中的纹理单元和opengl中的纹理单元（opengl一般提供16个纹理单元）
    // 告诉片段着色器，纹理texture1属于纹理单元0
    m_shaderProgram.setUniformValue("texture1", 0);
    // 告诉片段着色器，纹理texture2属于纹理单元1
    m_shaderProgram.setUniformValue("texture2", 1);

    // 关联顶点着色器中的变换矩阵采样器
    m_modelMatrix = m_shaderProgram.uniformLocation("model");
    m_viewMatrix = m_shaderProgram.uniformLocation("view");
    m_projectionMatrix = m_shaderProgram.uniformLocation("projection");

    // 设置纹理
    // 设置st方向上纹理超出坐标时的显示策略
    m_texture.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_texture.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // 设置纹理缩放时的策略
    m_texture.setMinificationFilter(QOpenGLTexture::Linear);
    m_texture.setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture.setData(QImage(":/wall.jpg"));

    // 设置st方向上纹理超出坐标时的显示策略
    m_texture2.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_texture2.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // 设置纹理缩放时的策略
    m_texture2.setMinificationFilter(QOpenGLTexture::Linear);
    m_texture2.setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture2.setData(QImage(":/face.png"));

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
    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 绑定纹理到纹理单元0
    m_texture.bind();
    glActiveTexture(GL_TEXTURE1);
    m_texture2.bind();

    // 我们这里只有一个shaderProgram和vao，其实没有必要重复bind和release
    // 这里只是为了教学，在多个shaderProgram的情况下，这是标准操作
    m_shaderProgram.bind();
    // 模型矩阵：将局部坐标转换为世界坐标
    QMatrix4x4 modelMatri;
    // 将物体绕x轴旋转-55度，相当于后仰
    modelMatri.rotate(-55.0f, 1.0f, 0.0f, 0.0f);

    // 视图矩阵：将世界坐标转换为观察坐标
    QMatrix4x4 viewMatri;
    // 将世界场景向前移动3（z -3）(相当于摄像机向后移动3)
    viewMatri.translate(0.0f, 0.0f, -3.0f);

    // 投影矩阵：将观察坐标转换为投影坐标
    QMatrix4x4 projectionMatri;
    // 这里透视投影中有对下面参数的介绍：https://learnopengl-cn.github.io/01%20Getting%20started/08%20Coordinate%20Systems/#3d
    projectionMatri.perspective(45.0f, width()/height(), 0.1f, 100.0f);

    m_shaderProgram.bind();
    // 更新变化矩阵的数据到gpu顶点着色器的采样器中
    m_shaderProgram.setUniformValue(m_modelMatrix, modelMatri);
    m_shaderProgram.setUniformValue(m_viewMatrix, viewMatri);
    m_shaderProgram.setUniformValue(m_projectionMatrix, projectionMatri);

    m_vao.bind();
    // glDrawElements会根据ebo中的6个索引去vbo中找顶点位置
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_shaderProgram.release();
}
