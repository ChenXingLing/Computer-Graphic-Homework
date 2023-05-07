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
std::vector<BYTE>c,A,E;
std::vector<std::vector<int > > dp,g,e,Q;
std::vector<int>d;
struct FA_{
    std::vector<int>pr,ne;int head,tail;
    FA_(){
        if(!pr.empty())pr.clear();
        if(!ne.empty())ne.clear();
        head=tail=0;
    }
    inline void creat(Re n){
        if(!pr.empty())pr.clear();
        if(!ne.empty())ne.clear();
        for(Re i=0;i<=n+1;++i)pr.push_back(0),ne.push_back(0);
        head=0,tail=n-1;
        //n+1,   0,1,2...,n-2,n-1,   n 
        ne[pr[0]=n+1]=0;for(Re i=1;i<=n-1;++i)pr[i]=i-1;
        for(Re i=0;i<=n-2;++i)ne[i]=i+1;pr[ne[n-1]=n]=n-1;
    }
    inline void del(Re x){
        if(x==head)head=ne[x];
        else if(x==tail){tail=pr[x];}
        else{
            Re pre=pr[x],nex=ne[x];pr[ne[pre]=nex]=pre;
        }
    }
    inline int pre(Re x){return x==head?-1:pr[x];}
    inline int nex(Re x){return x==tail?-1:ne[x];}
    inline int first(){return head;}
}I;
std::vector<FA_>F;
inline void calc(MyImage&img,MyImage&sal,int w,int h){
    Re n=img.height(),m=img.width(),K=img.dim();
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

    //【水平缩放】//
    if(!dp.empty())dp.clear();if(!g.empty())g.clear();
    dp=e,g=e;
    //链表初始化
    I.creat(m);if(!F.empty())F.clear();
    for(Re i=0;i<n;++i)F.push_back(I);
    Re add=abs(w-m);add=std::min(add,m);//最多允许扩大到原来的两倍
    if(!Q.empty())Q.clear();if(!d.empty())d.clear();
    for(Re O=0;O<add;++O)d.push_back(0);
    for(Re i=0;i<n;++i)Q.push_back(d);
    for(Re O=0;O<add;++O){//找竖直路径
        for(Re i=1;i<n;++i)
            for(Re j=F[i].first(),j_=F[i-1].first(),p;j!=-1;j=F[i].nex(j),j_=F[i-1].nex(j_)){
                Re tmp=dp[i-1][g[i][j]=j_];
                if((p=F[i-1].pre(j_))!=-1&&dp[i-1][p]<tmp)tmp=dp[i-1][g[i][j]=p];
                if((p=F[i-1].nex(j_))!=-1&&dp[i-1][p]<tmp)tmp=dp[i-1][g[i][j]=p];
                dp[i][j]=e[i][j]+tmp;
            }
        Re ans=dp[n-1][F[n-1].first()],pos=F[n-1].first();
        for(Re j=F[n-1].first();j!=-1;j=F[n-1].nex(j))if(dp[n-1][j]<ans)ans=dp[n-1][pos=j];//找到最小路径
        for(Re i=n-1;i>=0;--i)F[i].del(Q[i][O]=pos),pos=g[i][pos];//删除每行的pos，并记录
    }
    if(w>m){//水平放大
        int add=w-m;
        for(Re i=n-1;i>=0;--i){
            std::sort(Q[i].begin(),Q[i].end());
            //for(Re k=1;k<add;++k)if(Q[i][k]==Q[i][k-1])exit(0);
            Re p=m-1,ed=m-1+add;
            for(Re k=0;k<add;++k)a[i].push_back(a[i][0]),e[i].push_back(0);
            for(Re k=add-1;k>=0;--k){
                while(p>Q[i][k])e[i][ed]=e[i][p],a[i][ed--]=a[i][p--];
                //此时p==Q[k]
                //if(p!=Q[k])exit(0);
                if(Q[i][k]>0)e[i][ed]=e[i][p],a[i][ed--]=a[i][p--];//默认向左加边，p=Q[k]-1
                else p++;//在左边界时向右加边，p=Q[k]+1
                for(Re o=0;o<K;++o)a[i][ed][o]=(a[i][Q[i][k]][o]+a[i][p][o])/2;
                e[i][ed--]=(e[i][Q[i][k]]+e[i][p])/2;
            }
        }
    }

    //【输出图像】//
    if(!A.empty())A.clear();if(!E.empty())E.clear();
    for (int i=0;i<n;i++)
        if(w<m){
            for(int j=F[i].first();j!=-1;j=F[i].nex(j))
                for(int k=0;k<K;++k)A.push_back(a[i][j][k]),E.push_back(e[i][j]);
        }
        else{
            for(int j=0;j<m+add;++j)
			    for(int k=0;k<K;++k)A.push_back(a[i][j][k]),E.push_back(e[i][j]);
        }
    img=MyImage(A.data(),w,n,K,0,0),sal=MyImage(E.data(),w,n,K,0,0);
}
MyImage seamCarving(const MyImage& img,int w,int h){//水平缩放
    Re n=img.height(),m=img.width(),K=img.dim();
    MyImage sal("boy_saliency.png"),Ans=img;
    if(w!=m)calc(Ans,sal,w,h);
    if(h!=n)Ans.reverse(),sal.reverse(),calc(Ans,sal,h,w),Ans.reverse();
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

