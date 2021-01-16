#include "widget.h"
#include "ui_widget.h"

#include <QDebug>
#include <QTime>
#include <math.h>
/*
 * cpu根据时间计算颜色，然后通过uniform设置给片段着色器
 * uniform是cpu和片段着色器中交互数据的工具，不同于顶点坐标，它是全局的，在任何着色器中都可以使用
*/

float vertices[] = {
    0.5f, 0.5f, 0.0f,   // 右上角
    0.5f, -0.5f, 0.0f,  // 右下角
    -0.5f, -0.5f, 0.0f, // 左下角
    -0.5f, 0.5f, 0.0f   // 左上角
};

unsigned int indices[] = { // 注意索引从0开始!
    0, 1, 3, // 第一个三角形
    1, 2, 3  // 第二个三角形
};

const char* vertexShaderSource = R"(#version 330 core
                                 layout (location = 0) in vec3 aPos;

                                 void main()
                                 {
                                 gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
                                 })";

const char* fragmentShaderSource = R"(#version 330 core
                                   out vec4 FragColor;
                                   uniform vec4 ourColor; // 在程序代码中设定这个变量

                                   void main()
                                   {
                                   FragColor = ourColor;
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

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数名称，顶点坐标为float，起始偏移为0，顶点坐标类型为vec3，步幅为3个float
    m_shaderProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));
    // 启用顶点属性
    m_shaderProgram.enableAttributeArray(0);

    m_vao.release();
}

void Widget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void Widget::paintGL()
{
    // 为了循环触发绘制
    update();

    // 我们这里只有一个shaderProgram和vao，其实没有必要重复bind和release
    // 这里只是为了教学，在多个shaderProgram的情况下，这是标准操作
    m_shaderProgram.bind();

    // 动态调整片段着色器ourColor颜色
    float timeValue = QTime::currentTime().second();
    float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
    qDebug() << "greenValue:" << greenValue;

    QVector4D color(1.0f, greenValue, 0.0f, 1.0f);
    m_shaderProgram.setUniformValue("ourColor", color);

    m_vao.bind();
    // glDrawElements会根据ebo中的6个索引去vbo中找顶点位置
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    m_vao.release();
    m_shaderProgram.release();
}
