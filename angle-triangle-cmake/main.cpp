#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

WCHAR *szTitle = L"angle-triangle";
WCHAR *szWindowClass = L"angle-triangle";

HWND g_wnd{};
EGLDisplay g_display{};
EGLSurface g_surface{};
GLuint g_program{};

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL CreateWnd();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool InitEgl();
GLuint loadProgram(const GLchar *f_vertSource_p, const GLchar *f_fragSource_p);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {

  MyRegisterClass(hInstance);
  if (!CreateWnd()) {
    return FALSE;
  }

  if (!InitEgl()) {
    return FALSE;
  }

  // Load shader program
  constexpr char kVS[] = R"(attribute vec4 vPosition;
void main()
{
    gl_Position = vPosition;
})";

  constexpr char kFS[] = R"(precision mediump float;
void main()
{
    gl_FragColor = vec4(gl_FragCoord.x / 512.0, gl_FragCoord.y / 512.0, 0.0, 0.1);
})";

  g_program = loadProgram(kVS, kFS);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}

bool InitEgl() {
  EGLint configAttribList[] = {EGL_RED_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_ALPHA_SIZE,
                               8 /*: EGL_DONT_CARE*/,
                               EGL_DEPTH_SIZE,
                               EGL_DONT_CARE,
                               EGL_STENCIL_SIZE,
                               EGL_DONT_CARE,
                               EGL_SAMPLE_BUFFERS,
                               0,
                               EGL_NONE};
  EGLint surfaceAttribList[] = {
      // EGL_POST_SUB_BUFFER_SUPPORTED_NV, flags &
      // (ES_WINDOW_POST_SUB_BUFFER_SUPPORTED) ? EGL_TRUE : EGL_FALSE,
      EGL_POST_SUB_BUFFER_SUPPORTED_NV, EGL_FALSE, EGL_NONE, EGL_NONE};

  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;
  EGLContext context;
  EGLConfig config;
  EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE};

  EGLNativeWindowType hWnd = g_wnd;

  // Get Display
  g_display = eglGetDisplay(GetDC(hWnd)); // EGL_DEFAULT_DISPLAY
  if (g_display == EGL_NO_DISPLAY) {
    return EGL_FALSE;
  }

  // Initialize EGL
  if (!eglInitialize(g_display, &majorVersion, &minorVersion)) {
    return EGL_FALSE;
  }

  // Get configs
  if (!eglGetConfigs(g_display, NULL, 0, &numConfigs)) {
    return EGL_FALSE;
  }

  // Choose config
  if (!eglChooseConfig(g_display, configAttribList, &config, 1, &numConfigs)) {
    return EGL_FALSE;
  }

  // Create a surface
  g_surface = eglCreateWindowSurface(
      g_display, config, (EGLNativeWindowType)hWnd, surfaceAttribList);
  if (g_surface == EGL_NO_SURFACE) {
    return EGL_FALSE;
  }

  // Create a GL context
  context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, contextAttribs);
  if (context == EGL_NO_CONTEXT) {
    return EGL_FALSE;
  }

  // Make the context current
  if (!eglMakeCurrent(g_display, g_surface, g_surface, context)) {
    return EGL_FALSE;
  }
}

void printProgramLog(GLuint f_programId) {
  if (glIsProgram(f_programId)) {
    int logLen = 0;
    glGetProgramiv(f_programId, GL_INFO_LOG_LENGTH, &logLen);

    char *infoLog_a = new char[logLen];
    int infoLogLen = 0;
    glGetProgramInfoLog(f_programId, logLen, &infoLogLen, infoLog_a);

    std::cout << infoLog_a << std::endl;
    delete[] infoLog_a;
  }
}

void printShaderLog(GLuint f_shaderId) {
  if (glIsShader(f_shaderId)) {
    int logLen = 0;
    glGetShaderiv(f_shaderId, GL_INFO_LOG_LENGTH, &logLen);

    char *infoLog_a = new char[logLen];
    int infoLogLen = 0;
    glGetShaderInfoLog(f_shaderId, logLen, &infoLogLen, infoLog_a);

    std::cout << infoLog_a << std::endl;
    delete[] infoLog_a;
  }
}

GLuint loadShader(const GLchar *f_source_p, GLenum f_type) {
  GLuint shaderId = glCreateShader(f_type);
  glShaderSource(shaderId, 1, &f_source_p, nullptr);
  glCompileShader(shaderId);

  GLint compileStatus = GL_FALSE;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);

  if (!compileStatus) {
    printShaderLog(shaderId);
    glDeleteShader(shaderId);
    shaderId = 0;
  }

  return shaderId;
}

GLuint loadProgram(const GLchar *f_vertSource_p, const GLchar *f_fragSource_p) {
  GLuint vertShader = loadShader(f_vertSource_p, GL_VERTEX_SHADER);
  GLuint fragShader = loadShader(f_fragSource_p, GL_FRAGMENT_SHADER);

  if (!glIsShader(vertShader) || !glIsShader(fragShader)) {
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return 0;
  }

  GLuint programId = glCreateProgram();
  glAttachShader(programId, vertShader);
  glAttachShader(programId, fragShader);

  glLinkProgram(programId);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

  if (!linkStatus) {
    printProgramLog(programId);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteProgram(programId);
    return 0;
  }

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
  return programId;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
  case WM_PAINT: {
    RECT rcClient{};
    GetClientRect(g_wnd, &rcClient);

    glViewport(0, 0, rcClient.right - rcClient.left,
               rcClient.bottom - rcClient.top);
    // Clear
    glClearColor(0.2F, 0.2F, 0.2F, 1.F);
    glClear(GL_COLOR_BUFFER_BIT);
    // glViewport(0, 0, 512, 512);

    // Render scene
    GLfloat vertices[] = {
        0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f,
    };
    glUseProgram(g_program);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    eglSwapBuffers(g_display, g_surface);
  } break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEXW wcex{};

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wcex.lpszClassName = szWindowClass;

  return RegisterClassExW(&wcex);
}

BOOL CreateWnd() {
  HINSTANCE hInstance = GetModuleHandle(NULL);
  HWND hWnd =
      CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                    0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd) {
    return FALSE;
  }

  g_wnd = hWnd;

  ShowWindow(hWnd, SW_NORMAL);
  UpdateWindow(hWnd);

  return TRUE;
}
