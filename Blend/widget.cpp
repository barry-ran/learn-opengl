#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QImage>
#include <QTimer>
/*
 * 通过opengl blend依次叠加绘制多个纹理
 * 主要原理是一个一个的纹理绘制，通过opengl blend混合，而不是多个纹理一起绘制，使用shader混合
 * （相比使用多个纹理一起绘制，shader混合的方式，这种方式没有最大纹理单元数的限制）
 * opengl中blend原理参考这里 https://learnopengl-cn.github.io/04%20Advanced%20OpenGL/03%20Blending/
*/

float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    1.0f,  1.0f, 0.0f,   1.0f, 1.0f,   // 右上
    1.0f, -1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,   // 左下
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f    // 左上
};

unsigned int indices[] = { // 注意索引从0开始!
                           0, 1, 3, // 第一个三角形
                           1, 2, 3  // 第二个三角形
                         };

const char* vertexShaderSource = R"(#version 330 core
                                 layout (location = 0) in vec3 aPos; // 来自cpu的顶点坐标
                                 layout (location = 1) in vec2 aTexCoord; // 来自cpu的纹理坐标
                                 uniform mat4 matrix; // 接收来自cpu的变换矩阵数据，用来移动/缩放

                                 // 颜色和纹理坐标用于输出到片段着色器
                                 out vec2 TexCoord;

                                 void main()
                                 {
                                 // 原来的顶点坐标乘上变换矩阵，就可以达到变换效果（移动/缩放/旋转）
                                 gl_Position = matrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
                                 // 片段着色器采样图片是从下向上采样，所以图像是倒的，这里反转一个y
                                 TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
                                 })";

const char* fragmentShaderSource = R"(#version 330 core
                                   out vec4 FragColor;

                                   in vec2 TexCoord;

                                   uniform sampler2D texture1;

                                   void main()
                                   {
                                   // 混合纹理坐标/纹理数据/颜色
                                    FragColor = texture(texture1, TexCoord);
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

    // 关联顶点着色器中的变换矩阵采样器matrix
    m_matrixUniform = m_shaderProgram.uniformLocation("matrix");

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
    // 混合参数的具体含义可以参考 https://learnopengl-cn.github.io/04%20Advanced%20OpenGL/03%20Blending/
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 绘制第一张图片（wall）

    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 将第一张图片纹理绑定到纹理单元0
    m_texture.bind();

    // 调节绘制位置（旋转/缩放/移动）
    QMatrix4x4 modelview;
    // 移动（现移动再旋转和先旋转再移动效果是不一样的）
    //modelview.translate(1.0f, 0.0f, 0.0f);
    // 旋转（垂直于z轴的面逆时针旋转conut角度）
    //modelview.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    // 缩放
    //modelview.scale(0.9f);

    // 绑定program
    m_shaderProgram.bind();
    // 更新变化矩阵的数据到gpu顶点着色器的采样器matrix中
    m_shaderProgram.setUniformValue(m_matrixUniform, modelview);
    m_vao.bind();
    // 开始绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_shaderProgram.release();

    // 绘制第二张图片（face）
    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 将第二张图片纹理绑定到纹理单元0 （绘制完第一张，纹理单元0就可以继续使用了，这样就算绘制yuv，也最多只使用0，1，2三个纹理单元）
    m_texture2.bind();

    // 调节绘制位置（旋转/缩放/移动）
    QMatrix4x4 modelview2;
    // 缩放
    float scale = 0.5f;
    modelview2.scale(scale);
    // 移动（moveX和moveY为[-1，1]坐标系中期望目标左上角位置）
    float moveX = -1.0f;
    float moveY = 0.0f;

    // 移动受缩放的影响，所以这里根据缩放来转换一下
    moveX = (moveX - -1.0f * scale) / scale;
    moveY = (moveY - 1.0f * scale) / scale;

    modelview2.translate(moveX, moveY, 0.0f);
    // 旋转（垂直于z轴的面逆时针旋转conut角度）
    //modelview2.rotate(90.0f, 0.0f, 0.0f, 1.0f);

    // 绑定program
    m_shaderProgram.bind();
    // 更新变化矩阵的数据到gpu顶点着色器的采样器matrix中
    m_shaderProgram.setUniformValue(m_matrixUniform, modelview2);
    m_vao.bind();
    // 开始绘制
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_shaderProgram.release();
}
