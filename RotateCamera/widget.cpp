#include "widget.h"
#include "ui_widget.h"

#include <cmath>
#include <QDebug>
#include <QImage>
#include <QTimer>
/*
 * 通过调节观察矩阵，来调节我们的观看角度（camera）
 * opengl中camera原理参考这里 https://learnopengl-cn.github.io/01%20Getting%20started/09%20Camera/
*/
/*
float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
    0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
    -0.5f,  0.5f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
};
*/

// 36个顶点（6个面 x 每个面有2个三角形组成 x 每个三角形有3个顶点）
float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

unsigned int indices[] = { // 注意索引从0开始!
                           0, 1, 3, // 第一个三角形
                           1, 2, 3  // 第二个三角形
                         };

const char* vertexShaderSource = R"(#version 330 core
                                 layout (location = 0) in vec3 aPos; // 来自cpu的顶点坐标
                                 layout (location = 1) in vec2 aTexCoord; // 来自cpu的纹理坐标
                                 // 接收来自cpu的变换矩阵数据，用来移动/缩放
                                 uniform mat4 model;
                                 uniform mat4 view;
                                 uniform mat4 projection;

                                 // 颜色和纹理坐标用于输出到片段着色器
                                 out vec2 TexCoord;

                                 void main()
                                 {
                                 // 从右往左:局部坐标->世界坐标->观察坐标->裁剪坐标
                                 gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
                                 // 片段着色器采样图片是从下向上采样，所以图像是倒的，这里反转一个y
                                 TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
                                 })";

const char* fragmentShaderSource = R"(#version 330 core
                                   out vec4 FragColor;

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

    // 构造10个立方体的位置
    m_boxPositions.push_back(QVector3D(0.0f,  0.0f,  0.0f));
    m_boxPositions.push_back(QVector3D(2.0f,  5.0f, -15.0f));
    m_boxPositions.push_back(QVector3D(-1.5f, -2.2f, -2.5f));
    m_boxPositions.push_back(QVector3D(-3.8f, -2.0f, -12.3f));
    m_boxPositions.push_back(QVector3D(2.4f, -0.4f, -3.5f));
    m_boxPositions.push_back(QVector3D(-1.7f,  3.0f, -7.5f));
    m_boxPositions.push_back(QVector3D(1.3f, -2.0f, -2.5f));
    m_boxPositions.push_back(QVector3D(1.5f,  2.0f, -2.5f));
    m_boxPositions.push_back(QVector3D(1.5f,  0.2f, -1.5f));
    m_boxPositions.push_back(QVector3D(-1.3f,  1.0f, -1.5f));

    // 启动z缓冲，绘制3d立体场景需要
    glEnable(GL_DEPTH_TEST);
    // 黑色背景
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    // 同时清理颜色缓冲和深度缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    // 参数解释：顶点坐标在shader中的参数location，顶点坐标为float，起始偏移为0字节，顶点坐标类型为vec3，步幅为5个float
    m_shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0 * sizeof(float), 3, 5 * sizeof(float));
    // 启用顶点属性
    m_shaderProgram.enableAttributeArray(0);

    m_shaderProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    m_shaderProgram.enableAttributeArray(1);

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
    // 每隔100ms刷新一次
    static int count = 0;
    QTimer::singleShot(100, this, [this](){
        count += 1;
        update();
    });

    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 绑定纹理到纹理单元0
    m_texture.bind();
    glActiveTexture(GL_TEXTURE1);
    m_texture2.bind();

    // 我们这里只有一个shaderProgram和vao，其实没有必要重复bind和release
    // 这里只是为了教学，在多个shaderProgram的情况下，这是标准操作
    m_shaderProgram.bind();

    // 视图矩阵：将世界坐标转换为观察坐标
    QMatrix4x4 viewMatri;    
    // 让相机在一个圆上旋转
    float radius = 10.0f;
    float camX = sin(count) * radius;
    float camZ = cos(count) * radius;
    // 摄像机位置，一个目标位置和一个表示世界空间中的上向量的向量（我们计算右向量使用的那个上向量，详细原理介绍 https://learnopengl-cn.github.io/01%20Getting%20started/09%20Camera/
    viewMatri.lookAt(QVector3D(camX, 0.0, camZ), QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));

    // 投影矩阵：将观察坐标转换为投影坐标
    QMatrix4x4 projectionMatri;
    // 这里透视投影中有对下面参数的介绍：https://learnopengl-cn.github.io/01%20Getting%20started/08%20Coordinate%20Systems/#3d
    projectionMatri.perspective(45.0f, width()/height(), 0.1f, 100.0f);

    m_shaderProgram.bind();
    // 更新变化矩阵的数据到gpu顶点着色器的采样器中
    m_shaderProgram.setUniformValue(m_viewMatrix, viewMatri);
    m_shaderProgram.setUniformValue(m_projectionMatrix, projectionMatri);

    m_vao.bind();

    // 在不同位置绘制10个立方体
    for(unsigned int i = 0; i < 10; i++) {
        // 模型矩阵：将局部坐标转换为世界坐标
        QMatrix4x4 modelMatri;
        // 移动到指定位置
        modelMatri.translate(m_boxPositions[i]);
        // 旋转一定角度
        float angle = 20.0f * i + count;
        modelMatri.rotate(angle, 1.0f, 0.3f, 0.5f);

        m_shaderProgram.setUniformValue(m_modelMatrix, modelMatri);

        // 绘制一个立方体
        // glDrawArrays不使用顶点索引
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    m_vao.release();
    m_shaderProgram.release();
}
