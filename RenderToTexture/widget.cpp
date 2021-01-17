#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QImage>
#include <QElapsedTimer>

/*
 * 将两个图片渲染到纹理（离屏渲染），其中一个是大图片， 作为纹理大小，
 * 图片渲染到纹理以后，可以将纹理拿去上屏渲染（渲染大小为控件大小），渲染前可以在shader中做一些特效处理
 * 也可以同时readpix读取混合后到纹理数据（图片混合后的原尺寸大小）
*/

/***************************离屏渲染相关*****************************/

float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
    1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
    -1.0f,  1.0f, 0.0f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
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
                                   // 混合纹理坐标/纹理数据
                                   FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
                                   } )";

/***************************屏幕渲染相关*****************************/

float screenVertices[] = {
    //     ---- 位置 ----    - 纹理坐标 -
    0.9f,  0.9f, 0.0f,    1.0f, 1.0f,   // 右上
    0.9f, -0.9f, 0.0f,     1.0f, 0.0f,   // 右下
    -0.9f, -0.9f, 0.0f,     0.0f, 0.0f,   // 左下
    -0.9f,  0.9f, 0.0f,     0.0f, 1.0f    // 左上
};

const char* screenVertexShaderSource = R"(#version 330 core
                                       layout (location = 0) in vec2 aPos;
                                       layout (location = 1) in vec2 aTexCoords;

                                       out vec2 TexCoords;

                                       void main()
                                       {
                                       gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
                                       TexCoords = aTexCoords;
                                       })";

const char* screenFragmentShaderSource = R"(#version 330 core
                                         out vec4 FragColor;

                                         in vec2 TexCoords;

                                         uniform sampler2D screenTexture;

                                         void main()
                                         {
                                         //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
                                         FragColor = texture(screenTexture, TexCoords);
                                         // 特效处理：反相
                                         //FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);

                                         // 特效处理：灰度
                                         //FragColor = texture(screenTexture, TexCoords);
                                         //float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
                                         //FragColor = vec4(average, average, average, 1.0);
                                         })";

Widget::Widget(QWidget *parent) :
    QOpenGLWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    makeCurrent();
    m_offScreenVbo.destroy();
    m_offScreenEbo.destroy();
    m_offScreenVao.destroy();

    m_offScreenTexture.destroy();
    m_offScreenTexture2.destroy();

    if (m_offScreenFbo) {
        delete m_offScreenFbo;
    }

    doneCurrent();

    delete ui;
}

void Widget::initializeGL()
{
    qDebug() << "format:" << context()->format();

    initializeOpenGLFunctions();

    /***************************离屏渲染相关*****************************/

    QImage bg(":/bg.jpeg");
    m_offScreenSize = bg.size();
    // 默认创建一个GL_TEXTURE_2D的纹理附件的fbo，且纹理附件attach到GL_COLOR_ATTACHMENT0
    m_offScreenFbo = new QOpenGLFramebufferObject(m_offScreenSize);
    if (!m_offScreenFbo->isValid()) {
        qFatal("fbo invalid");
    }

    // vao初始化
    m_offScreenVao.create();
    // 绑定vao，下面对vbo/ebo的操作都保存到了vao上，下次绘制该物体直接m_vao.bind()即可，不用重复vbo/ebo的bind/allocate操作
    // vao也会保存ebo
    m_offScreenVao.bind();

    // vbo初始化
    m_offScreenVbo.create();
    m_offScreenVbo.bind();
    m_offScreenVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_offScreenVbo.allocate(vertices, sizeof(vertices));

    // ebo初始化
    m_offScreenEbo.create();
    m_offScreenEbo.bind();
    m_offScreenEbo.allocate(indices, sizeof(indices));

    // 编译着色器
    m_offScreenShaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_offScreenShaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_offScreenShaderProgram.link();
    m_offScreenShaderProgram.bind();

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数location，顶点坐标为float，起始偏移为0字节，顶点坐标类型为vec3，步幅为3个float
    m_offScreenShaderProgram.setAttributeBuffer(0, GL_FLOAT, 0 * sizeof(float), 3, 8 * sizeof(float));
    // 启用顶点属性
    m_offScreenShaderProgram.enableAttributeArray(0);

    m_offScreenShaderProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));
    m_offScreenShaderProgram.enableAttributeArray(1);

    m_offScreenShaderProgram.setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));
    m_offScreenShaderProgram.enableAttributeArray(2);

    // 关联片段着色器中的纹理单元和opengl中的纹理单元（opengl一般提供16个纹理单元）
    // 告诉片段着色器，纹理texture1属于纹理单元0
    m_offScreenShaderProgram.setUniformValue("texture1", 0);
    // 告诉片段着色器，纹理texture2属于纹理单元1
    m_offScreenShaderProgram.setUniformValue("texture2", 1);

    // 设置纹理
    // 设置st方向上纹理超出坐标时的显示策略
    m_offScreenTexture.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_offScreenTexture.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // 设置纹理缩放时的策略
    m_offScreenTexture.setMinificationFilter(QOpenGLTexture::Linear);
    m_offScreenTexture.setMagnificationFilter(QOpenGLTexture::Linear);
    m_offScreenTexture.setData(bg);

    // 设置st方向上纹理超出坐标时的显示策略
    m_offScreenTexture2.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_offScreenTexture2.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    // 设置纹理缩放时的策略
    m_offScreenTexture2.setMinificationFilter(QOpenGLTexture::Linear);
    m_offScreenTexture2.setMagnificationFilter(QOpenGLTexture::Linear);
    m_offScreenTexture2.setData(QImage(":/gyy.jpeg"));

    // 图片是rgba8888格式，注意这里的参数GL_RGBA8/GL_RGBA
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, wall.width(), wall.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, wall.bits());

    m_offScreenVao.release();
    qDebug() << m_offScreenShaderProgram.log();


    /***************************屏幕渲染相关*****************************/

    // vao初始化
    m_screenVao.create();
    // 绑定vao，下面对vbo/ebo的操作都保存到了vao上，下次绘制该物体直接m_vao.bind()即可，不用重复vbo/ebo的bind/allocate操作
    // vao也会保存ebo
    m_screenVao.bind();

    // vbo初始化
    m_screenVbo.create();
    m_screenVbo.bind();
    m_screenVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_screenVbo.allocate(screenVertices, sizeof(screenVertices));

    // ebo初始化
    m_screenEbo.create();
    m_screenEbo.bind();
    m_screenEbo.allocate(indices, sizeof(indices));

    // 编译着色器
    m_screenShaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, screenVertexShaderSource);
    m_screenShaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, screenFragmentShaderSource);
    m_screenShaderProgram.link();
    m_screenShaderProgram.bind();

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数location，顶点坐标为float，起始偏移为0字节，顶点坐标类型为vec3，步幅为3个float
    m_screenShaderProgram.setAttributeBuffer(0, GL_FLOAT, 0 * sizeof(float), 3, 5 * sizeof(float));
    // 启用顶点属性
    m_screenShaderProgram.enableAttributeArray(0);

    m_screenShaderProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    m_screenShaderProgram.enableAttributeArray(1);
}

void Widget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void Widget::paintGL()
{
    //update(); 开启循环paint，方便调试toImage耗时

    /***************************离屏渲染相关*****************************/

    // fbo绑定以后，后面所有的渲染都渲染到fbo的texture附件上了
    m_offScreenFbo->bind();
    // viewport设置为离屏texture大小，
    glViewport(0, 0, m_offScreenSize.width(), m_offScreenSize.height());
    // 清理背景
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 绑定纹理到纹理单元0
    m_offScreenTexture.bind();
    glActiveTexture(GL_TEXTURE1);
    m_offScreenTexture2.bind();

    m_offScreenShaderProgram.bind();
    m_offScreenVao.bind();
    // glDrawElements会根据ebo中的6个索引去vbo中找顶点位置
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_offScreenVao.release();
    m_offScreenShaderProgram.release();

    // 读取离屏数据
    QElapsedTimer t;
    t.start();
    //m_offScreenFbo->toImage().save("/Users/barry/test.jpg");
    // 性能还不错（25ms左右，渲染帧率30fps以内的话，不需要放到单独线程）,内部使用glReadPixels实现
    m_offScreenFbo->toImage();
    qDebug() << "toImage cost:" << t.elapsed() << " ms";


    /***************************屏幕渲染相关*****************************/

    // 恢复屏幕fbo
    m_offScreenFbo->release();
    // opengl glViewport使用设备像素，而width()/height()是逻辑像素，需要转换
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());
    // 清理背景
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    // 将fbo的texture渲染到屏幕
    glBindTexture(GL_TEXTURE_2D, m_offScreenFbo->texture());
    m_screenShaderProgram.bind();
    m_screenVao.bind();
    // glDrawElements会根据ebo中的6个索引去vbo中找顶点位置
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_screenVao.release();
    m_screenShaderProgram.release();
}
