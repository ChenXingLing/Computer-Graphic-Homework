#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#define FREEGLUT_STATIC
#include "gl_core_3_3.h"
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#define TW_STATIC
#include <AntTweakBar.h>

#include <ctime>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>

#include "objloader.h"
#include "glprogram.h"
#include "MyImage.h"
#include "VAOImage.h"
#include "VAOMesh.h"
#include "trackball.h"

#include "laplacian.h"


GLProgram MyMesh::prog, MyMesh::pickProg, MyMesh::pointSetProg;
GLTexture MyMesh::colormapTex;

MyMesh M;
int viewport[4] = { 0, 0, 1280, 960 };
int actPrimType = MyMesh::PE_VERTEX;

bool showATB = true;

using MatX3f = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>;
using MatX3i = Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor>;
MatX3f meshX;
MatX3i meshF;

void deform_preprocess()
{
    auto L = Laplacian(meshX, meshF);
}

#define Mat Matrix<float, Dynamic, Dynamic, RowMajor>
#define SPMat SparseMatrix<float, Eigen::ColMajor>
#define Re register int
#define S(a) ((a)*(a))
inline float dis(Re x,Re y){
    return sqrt(S(meshX(x,0)-meshX(y,0))+S(meshX(x,1)-meshX(y,1))+S(meshX(x,2)-meshX(y,2)));
}
inline float get_ankle(Re C,Re A,Re B){
    float a=dis(B,C),b=dis(A,C),c=dis(A,B);
    //printf("a=%5.3f, b=%5.3f, c=%5.3f\n",a,b,c);
    return acos( (S(a)+S(b)-S(c)) / (2.0*a*b) );
}
void meshDeform()
{
    using namespace Eigen;

    std::vector<int> Id = M.getConstrainVertexIds();
    std::vector<float> P = M.getConstrainVertexCoords();

    // TODO 2: compute deformation result y using Laplacian coordinates (使用拉普拉斯坐标计算变形结果 y)
    //meshX：点坐标，meshF：三角面
    int n=meshX.rows(),m=Id.size(),nf=meshF.rows();
    SPMat SPL(n+m,n),I(n,n),Lap(n+m,3),V(n,3),V_;
    std::vector<std::vector<int> >E1,E2;std::vector<int> tmpp,du;
    std::vector<std::vector<float> >W;std::vector<float> tmpp_,sw;
    for(Re i=0;i<n;++i)tmpp.push_back(-1),tmpp_.push_back(0),du.push_back(0),sw.push_back(0);
    for(Re i=0;i<n;++i)E1.push_back(tmpp),E2.push_back(tmpp),W.push_back(tmpp_);
    for(Re i=0;i<nf;++i){//根据三角面构建边信息，同时记录每条边所在的两个或一个三角面里的对角顶点。
        int a=meshF(i,0),b=meshF(i,1),c=meshF(i,2);
        if(E1[a][b]==-1)E1[a][b]=E1[b][a]=c,++du[a],++du[b];
        else E2[a][b]=E2[b][a]=c;
        if(E1[a][c]==-1)E1[a][c]=E1[c][a]=b,++du[a],++du[c];
        else E2[a][c]=E2[c][a]=b;
        if(E1[b][c]==-1)E1[b][c]=E1[c][b]=a,++du[b],++du[c];
        else E2[b][c]=E2[c][b]=a;
    }

    //计算L矩阵
    #define Tri Eigen::Triplet<float>
    #define up(_,__,___) ijv.push_back(Tri(_,__,___))
    std::vector<Tri > ijv;//三元组构造系数矩阵
    for(Re i=0;i<n;++i)up(i,i,1.0),V.insert(i,0)=meshX(i,0),V.insert(i,1)=meshX(i,1),V.insert(i,2)=meshX(i,2);
    //均匀权重：
    /*for(Re i=0;i<nf;++i){
        int a=meshF(i,0),b=meshF(i,1),c=meshF(i,2);float w;
        if(E1[a][b]==c)w=1.0/du[a],up(a,b,-w),w=1.0/du[b],up(b,a,-w);
        if(E1[a][c]==b)w=1.0/du[a],up(a,c,-w),w=1.0/du[c],up(c,a,-w);
        if(E1[b][c]==a)w=1.0/du[b],up(b,c,-w),w=1.0/du[c],up(c,b,-w);
    }*/
    //Cot权重
    for(Re i=0;i<nf;++i){
        int a=meshF(i,0),b=meshF(i,1),c=meshF(i,2);
        if(E1[a][b]==c){
            W[a][b]=1.0/tan(get_ankle(c,a,b));
            if(E2[a][b]!=-1)W[a][b]+=1.0/tan(get_ankle(E2[a][b],a,b));
            W[b][a]=W[a][b]; sw[a]+=W[a][b],sw[b]+=W[a][b];
        }
        if(E1[a][c]==b){
            W[a][c]=1.0/tan(get_ankle(b,a,c));
            if(E2[a][c]!=-1)W[a][c]+=1.0/tan(get_ankle(E2[a][c],a,c));
            W[c][a]=W[a][c]; sw[a]+=W[a][c],sw[c]+=W[a][c];
        }
        if(E1[b][c]==a){
            W[b][c]=1.0/tan(get_ankle(a,b,c));
            if(E2[b][c]!=-1)W[b][c]+=1.0/tan(get_ankle(E2[b][c],b,c));
            W[c][b]=W[b][c]; sw[b]+=W[b][c],sw[c]+=W[c][b];
        }
    }
    for(Re i=0;i<nf;++i){
        int a=meshF(i,0),b=meshF(i,1),c=meshF(i,2);float w;
        if(E1[a][b]==c)up(a,b,-W[a][b]/sw[a]),up(b,a,-W[a][b]/sw[b]);
        if(E1[a][c]==b)up(a,c,-W[a][c]/sw[a]),up(c,a,-W[a][c]/sw[c]);
        if(E1[b][c]==a)up(b,c,-W[b][c]/sw[b]),up(c,b,-W[b][c]/sw[c]);
    }
    SPL.setFromTriplets(ijv.cbegin(),ijv.cend());//生成L

    Lap=SPL*V;//计算Lap=LV
    for(Re i=0;i<m;++i)SPL.insert(n+i,Id[i])=1.0,Lap.insert(n+i,0)=P[i*3+0],Lap.insert(n+i,1)=P[i*3+1],Lap.insert(n+i,2)=P[i*3+2];//L和Lap增加约束行
    
    Eigen::SimplicialLLT<SPMat >solver;
    SPMat SPL_T=SPMat(SPL.transpose());//L的转置
    SPMat A=SPMat(SPL_T*SPL),b=SPMat(SPL_T*Lap);//A: LT*L, b:LT*Lap
    solver.compute(A); V_=solver.solve(b);//解 Ax=b
    //if(solver.info()!=Success){puts("fuck");return;}// decomposition failed
    Matrix<float, Dynamic, Dynamic, RowMajor> y=V_;
    
    /*
    printf("Id P:\n");for(Re i=0;i<m;++i)printf("%3d: %5.3f, %5.3f, %5.3f\n",Id[i],P[i*3+0],P[i*3+1],P[i*3+2]);
    printf("V:\n");for(Re i=0;i<meshX.rows();++i,puts(""))for(Re j=0;j<meshX.cols();++j)printf("%5.3f, ",meshX(i,j));
    printf("V_:\n");for(Re i=0;i<y.rows();++i,puts(""))for(Re j=0;j<y.cols();++j)printf("%5.3f, ",y(i,j));
    printf("E:\n");for(Re i=0;i<n;++i,puts(""))for(Re j=0;j<n;++j)printf("%3d",E1[i][j]==-1?0:1);
    printf("du:\n");for(Re i=0;i<n;++i)printf("du[%d]=%3d\n",i,du[i]);
    printf("sw:\n");for(Re i=0;i<n;++i)printf("sw[%d]=%5.3f\n",i,sw[i]);
    Mat L=SPL;printf("L:\n");for(Re i=0;i<n+m;++i,puts(""))for(Re j=0;j<n;++j)printf("%5.3f ",L(i,j));
    Mat LT=SPL_T;printf("LT:\n");for(Re i=0;i<n;++i,puts(""))for(Re j=0;j<n+m;++j)printf("%5.3f ",LT(i,j));
    Mat Lap_=Lap;printf("Lap:\n");for(Re i=0;i<n+m;++i)printf("%5.3f, %5.3f, %5.3f\n",Lap_(i,0),Lap_(i,1),Lap_(i,2));
    */

    for(Re i=0;i<m;++i)y(Id[i],0)=P[i*3+0],y(Id[i],1)=P[i*3+1],y(Id[i],2)=P[i*3+2];//约束点规定位置
    
    if (y.cols() > 3)  y = y.leftCols(3);
    if (y.rows() == 0 || y.cols() != 3) return;

    M.upload(y.data(), y.rows(), nullptr, 0, nullptr);
}

int mousePressButton;
int mouseButtonDown;
int mousePos[2];

bool msaa = true;

float myRotation[4] = {1, 0, 0, 0}; //BONUS: interactively specify the rotation for the Laplacian coordinates at the handles

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (msaa) glEnable(GL_MULTISAMPLE);
    else glDisable(GL_MULTISAMPLE);

    glViewport(0, 0, viewport[2], viewport[3]);
    M.draw(viewport);

    if (showATB) TwDraw();
    glutSwapBuffers();

    //glFinish();
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

        if (updown == GLUT_DOWN) {
            if (button == GLUT_LEFT_BUTTON) {
                if (glutGetModifiers()&GLUT_ACTIVE_CTRL) {
                }
                else {
                    int r = M.pick(x, y, viewport, M.PE_VERTEX, M.PO_ADD);
                }
            }
            else if (button == GLUT_RIGHT_BUTTON) {
                M.pick(x, y, viewport, M.PE_VERTEX, M.PO_REMOVE);
            }
        }
        else { // updown == GLUT_UP
            if (button == GLUT_LEFT_BUTTON);
        }

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
            else if (mousePressButton == GLUT_LEFT_BUTTON) {
                if (!M.moveCurrentVertex(x, y, viewport)) {
                    meshDeform();
                }
                else {
                    const float s[2] = { 2.f / viewport[2], 2.f / viewport[3] };
                    auto r = Quat<float>(M.mQRotate)*Quat<float>::trackball(x*s[0] - 1, 1 - y*s[1], s[0]*mousePos[0] - 1, 1 - s[1]*mousePos[1]);
                    std::copy_n(r.q, 4, M.mQRotate);
                }
            }
        }
    }

    mousePos[0] = x; mousePos[1] = y;

    glutPostRedisplay();
}


void onMouseWheel(int wheel_number, int direction, int x, int y)
{
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


void createTweakbar()
{
    TwBar *bar = TwGetBarByName("MeshViewer");
    if (bar)    TwDeleteBar(bar);

    //Create a tweak bar
    bar = TwNewBar("MeshViewer");
    TwDefine(" MeshViewer size='220 100' color='0 128 255' text=dark alpha=128 position='5 5'"); // change default tweak bar size and color

    TwAddVarRO(bar, "#Vertex", TW_TYPE_INT32, &M.nVertex, " group='Mesh View'");
    TwAddVarRO(bar, "#Face", TW_TYPE_INT32, &M.nFace, " group='Mesh View'");

    TwAddVarRW(bar, "Point Size", TW_TYPE_FLOAT, &M.pointSize, " group='Mesh View' ");
    TwAddVarRW(bar, "Vertex Color", TW_TYPE_COLOR4F, M.vertexColor.data(), " group='Mesh View' help='mesh vertex color' ");


    TwAddVarRW(bar, "Edge Width", TW_TYPE_FLOAT, &M.edgeWidth, " group='Mesh View' ");
    TwAddVarRW(bar, "Edge Color", TW_TYPE_COLOR4F, M.edgeColor.data(), " group='Mesh View' help='mesh edge color' ");

    TwAddVarRW(bar, "Face Color", TW_TYPE_COLOR4F, M.faceColor.data(), " group='Mesh View' help='mesh face color' ");

    TwDefine(" MeshViewer/'Mesh View' opened=false ");

    TwAddVarRW(bar, "Rotation", TW_TYPE_QUAT4F, myRotation, " group='Modeling' open help='can be used to specify the rotation for current handle' ");
}

int main(int argc, char *argv[])
{
    if (initGL(argc, argv)) {
        fprintf(stderr, "!Failed to initialize OpenGL!Exit...");
        exit(-1);
    }

    MyMesh::buildShaders();

    std::vector<float> x;
    std::vector<int> f;

    const char* meshpath = argc > 1 ? argv[1] : "balls.obj";
    readObj(meshpath, x, f);

    meshX = Eigen::Map<MatX3f>(x.data(), x.size() / 3, 3);
    meshF = Eigen::Map<MatX3i>(f.data(), f.size() / 3, 3);


    M.upload(x.data(), x.size() / 3, f.data(), f.size() / 3, nullptr);

    //////////////////////////////////////////////////////////////////////////
    TwInit(TW_OPENGL_CORE, NULL);
    //Send 'glutGetModifers' function pointer to AntTweakBar;
    //required because the GLUT key event functions do not report key modifiers states.
    TwGLUTModifiersFunc(glutGetModifiers);
    glutSpecialFunc([](int key, int x, int y) { TwEventSpecialGLUT(key, x, y); glutPostRedisplay(); }); // important for special keys like UP/DOWN/LEFT/RIGHT ...
    TwCopyStdStringToClientFunc([](std::string& dst, const std::string& src) {dst = src; });

    createTweakbar();

    //////////////////////////////////////////////////////////////////////////
    atexit([] { TwDeleteAllBars();  TwTerminate(); }); 

    glutTimerFunc(1, [](int) {
        deform_preprocess();
    }, 
        0);


    //////////////////////////////////////////////////////////////////////////
    glutMainLoop();

    return 0;
}
