//   Copyright © 2021, Renjie Chen @ USTC

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#define FREEGLUT_STATIC
#include "gl_core_3_3.h"
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#define TW_STATIC
#include <AntTweakBar.h>

#include <vector>
#include <string>

#include "glprogram.h"
#include "MyImage.h"
#include "VAOImage.h"
#include "VAOMesh.h"


GLProgram MyMesh::prog;

MyMesh M;
int viewport[4] = { 0, 0, 1280, 960 };

bool showATB = true;

std::string imagefile = "boy.png";

MyImage img;
int resize_width, resize_height;

int mousePressButton;
int mouseButtonDown;
int mousePos[2];

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, viewport[2], viewport[3]);
    M.draw(viewport);

    if (showATB) TwDraw();
    glutSwapBuffers();
}

void onKeyboard(unsigned char code, int x, int y)
{
    if (!TwEventKeyboardGLUT(code, x, y)) {
        switch (code) {
        case 17:
            exit(0);
        case 'f':
            glutFullScreenToggle();
            break;
        case ' ':
            showATB = !showATB;
            break;
        }
    }

    glutPostRedisplay();
}

void onMouseButton(int button, int updown, int x, int y)
{
    if (!showATB || !TwEventMouseButtonGLUT(button, updown, x, y)) {
        mousePressButton = button;
        mouseButtonDown = updown;

        mousePos[0] = x;
        mousePos[1] = y;
    }

    glutPostRedisplay();
}


void onMouseMove(int x, int y)
{
    if (!showATB || !TwEventMouseMotionGLUT(x, y)) {
        if (mouseButtonDown == GLUT_DOWN) {
            if (mousePressButton == GLUT_MIDDLE_BUTTON) {
                M.moveInScreen(mousePos[0], mousePos[1], x, y, viewport);
            }
        }
    }

    mousePos[0] = x; mousePos[1] = y;

    glutPostRedisplay();
}


void onMouseWheel(int wheel_number, int direction, int x, int y)
{
    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) {
    }
    else
        M.mMeshScale *= direction > 0 ? 1.1f : 0.9f;

    glutPostRedisplay();
}

int initGL(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize(960, 960);
    glutInitWindowPosition(200, 50);
    glutCreateWindow(argv[0]);

    // !Load the OpenGL functions. after the opengl context has been created
    if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
        return -1;

    glClearColor(1.f, 1.f, 1.f, 0.f);

    glutReshapeFunc([](int w, int h) { viewport[2] = w; viewport[3] = h; TwWindowSize(w, h); });
    glutDisplayFunc(display);
    glutKeyboardFunc(onKeyboard);
    glutMouseFunc(onMouseButton);
    glutMotionFunc(onMouseMove);
    glutMouseWheelFunc(onMouseWheel);
    glutCloseFunc([]() {exit(0); });
    return 0;
}

void uploadImage(const MyImage& img)
{
    int w = img.width();
    int h = img.height();
    float x[] = { 0, 0, 0, w, 0, 0, w, h, 0, 0, h, 0 };
    M.upload(x, 4, nullptr, 0, nullptr);

    M.tex.setImage(img);
    M.tex.setClamping(GL_CLAMP_TO_EDGE);
}


void readImage(const std::string& file)
{
    int w0 = img.width(), h0 = img.height();
    img = MyImage(file);
    uploadImage(img);
    resize_width = img.width();
    resize_height = img.height();

    if (w0 != img.width() || h0 != img.height()) M.updateBBox();
}
#define Re register int
std::vector<std::vector<std::vector<BYTE> > > a;
std::vector<std::vector<BYTE> >b;
std::vector<BYTE>c,A;
std::vector<std::vector<int > > dp,g,e;
std::vector<int>d;
MyImage seamCarving(const MyImage& img, int w, int h)//(i,j) <-> (h,w) <-> (竖直高度，水平宽度 )
{
    Re n=img.height(),m=img.width(),K=img.dim();

    MyImage sal("boy_saliency.png");
    
    //【拷贝信息】//
    if(!a.empty())a.clear();if(!e.empty())e.clear();
    for(Re i=0;i<n;++i){
        if(!b.empty())b.clear();
        for(Re j=0;j<m;++j){
            if(!c.empty())c.clear();
            for(Re k=0;k<K;++k)c.push_back(img.data()[(i*m+j)*K+k]);
            b.push_back(c);
        }
        a.push_back(b);

        if(!d.empty())d.clear();
        for(Re j=0;j<m;++j)d.push_back(sal.data()[(i*m+j)*K+0]);
        e.push_back(d);
    }

    //【水平缩小】//
    if(!dp.empty())dp.clear();if(!g.empty())g.clear();
    dp=e,g=e;
    for(;w<m;--m){//找竖直路径
        dp[0]=e[0];//初始化第0行
        for(Re i=1;i<n;++i)
            for(Re j=0;j<m;++j){
                dp[i][j]=e[i][j]+dp[i-1][g[i][j]=j];
                if(j>0&&dp[i-1][j-1]<dp[i-1][j])dp[i][j]=e[i][j]+dp[i-1][g[i][j]=j-1];
                if(j<m-1&&e[i][j]+dp[i-1][j+1]<dp[i][j])dp[i][j]=e[i][j]+dp[i-1][g[i][j]=j+1];
            }
        int ans=dp[n-1][0],pos=0;
        for(Re j=1;j<m;++j)if(dp[n-1][j]<ans)ans=dp[n-1][pos=j];//找到最小路径
        //把每行的pos挪到最后一列删除
        for(Re j=pos;j<m-1;++j)swap(a[n-1][j],a[n-1][j+1]),std::swap(e[n-1][j],e[n-1][j+1]);
        a[n-1].pop_back(),e[n-1].pop_back();
        for(Re i=n-2;i>=0;--i){
            pos=g[i+1][pos];
            for(Re j=pos;j<m-1;++j)swap(a[i][j],a[i][j+1]),std::swap(e[i][j],e[i][j+1]);
            a[i].pop_back();
        }
    }

    //【竖直缩小】//
    if(!dp.empty())dp.clear();if(!g.empty())g.clear();
    dp=e,g=e;
    for(;h<n;--n){//找水平路径
        for(Re i=0;i<n;++i)dp[i][0]=e[i][0];//初始化第0列
        for(Re j=1;j<m;++j)
            for(Re i=0;i<n;++i){
                dp[i][j]=e[i][j]+dp[g[i][j]=i][j-1];
                if(i>0&&dp[i-1][j-1]<dp[i][j-1])dp[i][j]=e[i][j]+dp[g[i][j]=i-1][j-1];
                if(i<n-1&&e[i][j]+dp[i+1][j-1]<dp[i][j])dp[i][j]=e[i][j]+dp[g[i][j]=i+1][j-1];
            }
        int ans=dp[0][m-1],pos=0;
        for(Re i=1;i<n;++i)if(dp[i][m-1]<ans)ans=dp[pos=i][m-1];//找到最小路径
        //把每列的pos挪到最后一行删除
        for(Re i=pos;i<n-1;++i)swap(a[i][m-1],a[i+1][m-1]),std::swap(e[i][m-1],e[i+1][m-1]);
        for(Re j=m-2;j>=0;--j){
            pos=g[pos][j+1];
            for(Re i=pos;i<n-1;++i)swap(a[i][j],a[i+1][j]),std::swap(e[i][j],e[i+1][j]);
        }
        a.pop_back(),e.pop_back();
    }

    //【水平放大】//

    //【输出图像】//
    if(!A.empty())A.clear();
    for (int i=0;i<n;i++)
		for(int j=0;j<m;++j)
			for(int k=0;k<K;++k)
                A.push_back(a[i][j][k]);
    MyImage Ans(A.data(),m,n,K,0,0);
    return Ans;
}

void createTweakbar()
{
    //Create a tweak bar
    TwBar *bar = TwNewBar("Image Viewer");
    TwDefine(" 'Image Viewer' size='220 150' color='0 128 255' text=dark alpha=128 position='5 5'"); // change default tweak bar size and color

    TwAddVarRW(bar, "Scale", TW_TYPE_FLOAT, &M.mMeshScale, " min=0 step=0.1");

    TwAddVarRW(bar, "Image filename", TW_TYPE_STDSTRING, &imagefile, " ");
    TwAddButton(bar, "Read Image", [](void*) { readImage(imagefile); }, nullptr, "");

    TwAddVarRW(bar, "Resize Width", TW_TYPE_INT32, &resize_width, "group='Seam Carving' min=1 ");
    TwAddVarRW(bar, "Resize Height", TW_TYPE_INT32, &resize_height, "group='Seam Carving' min=1 ");
    TwAddButton(bar, "Run Seam Carving", [](void* img) {
        MyImage newimg = seamCarving(*(const MyImage*)img, resize_width, resize_height);
        uploadImage(newimg);
        }, 
        &img, "");
}


int main(int argc, char *argv[])
{
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { 100, 5000 });

    if (initGL(argc, argv)) {
        fprintf(stderr, "!Failed to initialize OpenGL!Exit...");
        exit(-1);
    }

    MyMesh::buildShaders();


    float x[] = { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0 };
    float uv[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
    int t[] = { 0, 1, 2, 2, 3, 0 };

    M.upload(x, 4, t, 2, uv);

    //////////////////////////////////////////////////////////////////////////
    TwInit(TW_OPENGL_CORE, NULL);
    //Send 'glutGetModifers' function pointer to AntTweakBar;
    //required because the GLUT key event functions do not report key modifiers states.
    TwGLUTModifiersFunc(glutGetModifiers);
    glutSpecialFunc([](int key, int x, int y) { TwEventSpecialGLUT(key, x, y); glutPostRedisplay(); }); // important for special keys like UP/DOWN/LEFT/RIGHT ...
    TwCopyStdStringToClientFunc([](std::string& dst, const std::string& src) {dst = src; });

    createTweakbar();

    //////////////////////////////////////////////////////////////////////////
    atexit([] { TwDeleteAllBars();  TwTerminate(); });  // Called after glutMainLoop ends

    glutTimerFunc(1, [](int) { readImage(imagefile); },  0);


    //////////////////////////////////////////////////////////////////////////
    glutMainLoop();

    return 0;
}

