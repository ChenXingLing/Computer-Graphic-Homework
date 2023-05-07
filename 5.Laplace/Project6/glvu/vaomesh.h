#pragma once
#include <algorithm>
#include <vector>
#include <array>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/LU>
#include "glprogram.h"
#include "glarray.h"


Eigen::Matrix3f quaternaion2matrix(const float *q)
{
    double s = Eigen::Map<const Eigen::Vector4f>(q).squaredNorm();
    Eigen::Matrix3f res;

    float *m = res.data();

    m[0] = 1.f - 2.f * (q[1] * q[1] + q[2] * q[2]);
    m[1] = 2.f * (q[0] * q[1] - q[2] * q[3]);
    m[2] = 2.f * (q[2] * q[0] + q[1] * q[3]);

    m[3 + 0] = 2.f * (q[0] * q[1] + q[2] * q[3]);
    m[3 + 1] = 1.f - 2.f * (q[2] * q[2] + q[0] * q[0]);
    m[3 + 2] = 2.f * (q[1] * q[2] - q[0] * q[3]);

    m[6 + 0] = 2.f * (q[2] * q[0] - q[1] * q[3]);
    m[6 + 1] = 2.f * (q[1] * q[2] + q[0] * q[3]);
    m[6 + 2] = 1.f - 2.f * (q[1] * q[1] + q[0] * q[0]);

    return res.transpose();
}

Eigen::Matrix4f perspective(float fovy,  float aspect,  float zNear, float zFar)
{
    assert(aspect > 0);
    assert(zFar > zNear);

    float radf = fovy / 180 * M_PI;
    float tanHalfFovy = tan(radf / 2);
    Eigen::Matrix4f res = Eigen::Matrix4f::Zero();
    res(0, 0) = 1 / (aspect * tanHalfFovy);
    res(1, 1) = 1 / (tanHalfFovy);
    res(2, 2) = -(zFar + zNear) / (zFar - zNear);
    res(3, 2) = -1;
    res(2, 3) = -(2 * zFar * zNear) / (zFar - zNear);
    return res;
}

Eigen::Matrix4f lookAt(const float* eye, const float* center, const float* up)
{
    using Vec = Eigen::RowVector3f;
    using MapVec = Eigen::Map<const Vec>;
    Vec f = (MapVec(center) - MapVec(eye)).normalized();
    Vec u = MapVec(up).normalized();
    Vec s = f.cross(u).normalized();
    u = s.cross(f);

    Eigen::Matrix4f res;
    res.leftCols(3) << s, u, -f, 0, 0, 0;
    res.rightCols(1) << -res.topLeftCorner(3, 3)*MapVec(eye).transpose(), 1;
    return res;
}

template<typename R = float, int dimension = 3>
struct GLMesh
{
    enum { dim = dimension };
    using MapMat4 = Eigen::Map < Eigen::Matrix < float, 4, 4, Eigen::RowMajor > >;
    using ConstMat4 = Eigen::Map < const Eigen::Matrix < float, 4, 4, Eigen::RowMajor > >;

    enum PickingElements { PE_NONE = 0, PE_VERTEX, PE_FACE };
    enum PickingOperations { PO_NONE = 0, PO_ADD, PO_REMOVE };

    struct Mesh
    {
        std::vector<R> X, UV;
        std::vector<int> T;
        size_t nVertex() const { return X.size() / dim; }
        size_t nFace() const { return T.size() / 3; }


    };
    Mesh mesh;
    GLTexture tex;
    static GLTexture colormapTex;

    typedef std::array<R, 4> vec4;
    typedef std::array<R, dim> vec;

    int nVertex;
    int nFace;
    GLuint vaoHandle;

    GLArray<R, dim> gpuX;
    GLArray<int, 3, true> gpuT;
    GLArray<R, 2>   gpuUV;
    GLArray<R, 1>   gpuVertexData;
    GLArray<R, 1>   gpuFaceData;
    R vtxDataMinMax[2];
    bool vizVtxData;

    std::map<int, vec> constrainVertices;
    std::vector<R> vertexData;
    std::vector<R> faceData;
    std::vector<R> vertexVF;

    int actVertex;
    int actFace;
    int actConstrainVertex;

    vec4 faceColor = { 1.f, 1.f, 1.f, 1.f };
    vec4 edgeColor = { 0.f, 0.f, 0.f, 0.8f };
    vec4 vertexColor = { 1.f, 0.f, 0.f, .8f };
    int depthMode = 0;
    float edgeWidth = 1.f;
    float pointSize = 0.f;

    float auxPointSize;
    float vertexDataDrawScale;
    float faceDataDrawScale;
    float VFDrawScale = 0.f;

    float mMeshScale = 1.f;
    float mTextureScale = 1.f;

    std::vector<int> auxVtxIdxs;

    vec mTranslate = { 0, 0, 0 };
    float mQRotate[4] = { 0,0,0,1 };
    float mViewCenter[3] = { 0, 0, 0 };
    R bbox[dim * 2];

    bool showTexture = false;
    bool drawTargetP2P = true;

    static GLProgram prog, pickProg, pointSetProg;

    ~GLMesh() { glDeleteVertexArrays(1, &vaoHandle); }

    GLMesh() :vaoHandle(0), actVertex(-1), actFace(-1),
        actConstrainVertex(-1), vertexDataDrawScale(0.f),
        faceDataDrawScale(0.f), vizVtxData(0), auxPointSize(0.f)
    {}

    void allocateStorage(int nv, int nf)
    {
        if (nv == nVertex && nf == nFace) return;
        nVertex = nv;
        nFace = nf;

        if (!vaoHandle)  glGenVertexArrays(1, &vaoHandle);

        glBindVertexArray(vaoHandle);

        gpuX.allocateStorage(nv);
        glVertexAttribPointer(0, dim, GLType<R>::val, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);  // Vertex position

        gpuUV.allocateStorage(nv);
        glVertexAttribPointer(2, 2, GLType<R>::val, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);  // Texture coords

        gpuVertexData.allocateStorage(nv);
        glVertexAttribPointer(4, 1, GLType<R>::val, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(4);  // Vertex data

        gpuT.allocateStorage(nf);

        glBindVertexArray(0);
    }

    std::vector<int> getConstrainVertexIds() const {
        std::vector<int> idxs;
        idxs.reserve(constrainVertices.size());
        for (auto it : constrainVertices)   idxs.push_back(it.first);
        return idxs;
    }

    std::vector<R> getConstrainVertexCoords() const {
        std::vector<R> x;
        x.reserve(constrainVertices.size()*dim);
        for (auto it : constrainVertices) {
            x.insert(x.end(), { it.second[0], it.second[1], it.second[2] });
        }
        return x;
    }

    void getTriangulation(int *t) { gpuT.downloadData(t, nFace); }

    void getTriangulation(int ti, int *t) { gpuT.at(ti, t); }

    void getVertex(R *x) { gpuX.downloadData(x, nVertex); }

    void getVertex(int vi, R *x) { gpuX.at(vi, x); }

    void setVertex(int vi, const R *x)
    {
        //for (int i = 0; i < dim; i++)    mesh.X[vi*dim + i] = x[i];
        gpuX.setAt(vi, x);
    }

    void setConstraintVertices(const int *ids, const R* pos, size_t nc) {
        constrainVertices.clear();
        for (size_t i = 0; i < nc; i++) constrainVertices.insert({ ids[i], vec{ pos[i * 2], pos[i * 2 + 1], 0 } });
    }


    void setVertexDataViz(const R* val)
    {
        if (val) {
            gpuVertexData.uploadData(val, mesh.nVertex());

            glBindVertexArray(vaoHandle);
            gpuVertexData.bind();
            glVertexAttribPointer(5, 1, GLType<R>::val, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(5);

            auto mm = std::minmax_element(val, val + mesh.nVertex());
            vtxDataMinMax[0] = *mm.first;
            vtxDataMinMax[1] = *mm.second;
            prog.bind();
            prog.setUniform("dataMinMax", vtxDataMinMax[0], vtxDataMinMax[1]);
        }
        else {
            glBindVertexArray(vaoHandle);
            glDisableVertexAttribArray(5);
        }
    }

    void updateDataVizMinMax()
    {
        prog.bind();
        prog.setUniform("dataMinMax", vtxDataMinMax[0], vtxDataMinMax[1]);
    }

    template<class MatrixR, class MatrixI>
    void upload(const MatrixR &x, const MatrixI &t, const R *uv)
    {
        upload(x.data(), (int)x.rows(), t.count() ? t.data() : nullptr, (int)t.rows(), uv);
    }

    void upload(const R *x, int nv, const int *t, int nf, const R *uv)
    {
        allocateStorage(x ? nv : nVertex, t ? nf : nFace);

        if (x) { gpuX.uploadData(x, nv, false); mesh.X.assign(x, x + nv*dim); }
        if (uv) { gpuUV.uploadData(uv, nv, false); mesh.UV.assign(uv, uv + nv * 2); }
        if (t) { gpuT.uploadData(t, nf, false); mesh.T.assign(t, t + nf * 3); }

        if (x&&t) {
            boundingbox(x, nv, bbox);  // update bounding box for the initialization
            constrainVertices.clear();
            auxVtxIdxs.clear();

            actVertex = -1;
            actFace = -1;
            actConstrainVertex = -1;

            vertexData = std::vector<R>(nv, 0);
            faceData = std::vector<R>(nf, 1);

            gpuVertexData.uploadData(vertexData.data(), nVertex, false);
            gpuFaceData.uploadData(faceData.data(), nFace);
        }

    }

    void updateBBox() {
        boundingbox(mesh.X.data(), nVertex, bbox);
    }

    std::vector<R> baryCenters(const R* X)
    {
        std::vector<R> x(nFace * dim);
        for (int i = 0; i < nFace; i++) {
            const R *px[] = { &X[mesh.T[i * 3] * dim], &X[mesh.T[i * 3 + 1] * dim], &X[mesh.T[i * 3 + 2] * dim] };
            for (int j = 0; j < dim; j++) x[i*dim + j] = (px[0][j] + px[1][j] + px[2][j]) / 3;
        }

        return x;
    }


    float actVertexData() const { return (actVertex >= 0 && actVertex < nVertex) ? vertexData[actVertex] : std::numeric_limits<float>::infinity(); }
    float actFaceData() const { return (actFace >= 0 && actFace < nFace) ? faceData[actFace] : std::numeric_limits<float>::infinity(); }
    void incActVertexData(float pct) { setVertexData(actVertex, (vertexData[actVertex] + 1e-3f) * (1 + pct)); }
    void incActFaceData(float pct) { setFaceData(actFace, (faceData[actFace] + 1e-3f) * (1 + pct)); }

    void setVertexData(int i, R v) {
        MAKESURE(i < nVertex && i >= 0);
        gpuVertexData.setAt(i, &v);
        vertexData[i] = v;
    }

    void setFaceData(int i, R v) {
        MAKESURE(i < nFace && i >= 0);
        gpuFaceData.setAt(i, &v);
        faceData[i] = v;
    }

    void setVertexData(const R *vtxData) { gpuVertexData.uploadData(vtxData, nVertex, false); }
    void setFaceData(const R *vtxData) { gpuFaceData.uploadData(vtxData, nFace, false); }

    bool showWireframe() const { return edgeWidth > 0; }
    bool showVertices() const { return pointSize > 0; }

    R drawscale() const
    {
        R scale0 = 1.9f / std::max(std::max(bbox[dim] - bbox[0], bbox[1 + dim] - bbox[1]), bbox[2 + dim] - bbox[2]);
        return mMeshScale*scale0;
    }

    std::array<R, 16> matMVP(const int *vp, bool offsetDepth = false, bool colmajor = false) const {
        R trans[] = { (bbox[0] + bbox[dim]) / 2, (bbox[1] + bbox[1 + dim]) / 2, (bbox[2] + bbox[2 + dim]) / 2 };
        R ss = drawscale(); 
        R t[] = { mTranslate[0] - trans[0], mTranslate[1] - trans[1], mTranslate[2] - trans[2] };


        using Mat4 = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

        Mat4 proj = perspective(45.f, vp[2] / float(vp[3]), 0.1f, 100.0f);
        if(offsetDepth)  proj(2, 2) += 1e-4;
  
        float campos[] = { 0, 0, 4 }, up[] = { 0, 1, 0 };// Head is up (set to 0,-1,0 to look upside-down)
        Mat4 view = lookAt(campos, mViewCenter, up);

        Mat4 model;
        model<<ss*quaternaion2matrix(mQRotate), Eigen::Map<Eigen::Array3f>(t)*ss,
                 0,  0,  0,   1;
  
        std::array<R,16> mvp;
        Eigen::Map<Mat4>(mvp.data()) = proj*view*model;

        if (colmajor) Eigen::Map<Mat4>(mvp.data()).transposeInPlace();
        return mvp;
    }

    void moveInScreen(int x0, int y0, int x1, int y1, int *vp) {
        float dx = (x1 - x0) * 2.f / vp[2];
        float dy = -(y1 - y0) * 2.f / vp[3];

        mViewCenter[0] -= dx;
        mViewCenter[1] -= dy;
    }


    void draw(const int *vp)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        tex.bind();

        prog.bind();
        prog.setUniform("textureScale", mTextureScale);
        prog.setUniform("MVP", matMVP(vp).data());
        prog.setUniform("useTexMap", int(showTexture));
        prog.setUniform("color", faceColor.data());

        glBindVertexArray(vaoHandle);

        if (vizVtxData) {
            prog.setUniform("colorCoding", int(vizVtxData));
            prog.setUniform("useTexMap", 1);
            colormapTex.bind();
        }

        glDrawElements(GL_TRIANGLES, 3 * nFace, GL_UNSIGNED_INT, nullptr); // not GL_INT

        if (vizVtxData) {
            prog.setUniform("colorCoding", 0);
            prog.setUniform("useTexMap", int(showTexture));
            tex.bind();
        }

        //glDisable(GL_DEPTH_TEST);
            
        prog.setUniform("MVP", matMVP(vp, true).data());
        if (showWireframe()) {
            glLineWidth(edgeWidth);
            prog.setUniform("color", edgeColor.data());
            prog.setUniform("useTexMap", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, 3 * nFace, GL_UNSIGNED_INT, nullptr); // not GL_INT
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (showVertices()) {
            glPointSize(pointSize*mMeshScale);
            prog.setUniform("useTexMap", 0);
            prog.setUniform("color", vertexColor.data());
            glDrawArrays(GL_POINTS, 0, nVertex);
        }


        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        prog.bind();
        if (!constrainVertices.empty()) {
            prog.setUniform("useTexMap", 0);

            glDisable(GL_PROGRAM_POINT_SIZE);
            glPointSize(pointSize + 12);

            const auto idxs = getConstrainVertexIds();
            GLArray<R, dim> consX(getConstrainVertexCoords().data(), idxs.size());

            glVertexAttribPointer(0, dim, GLType<R>::val, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);  // Vertex position
            prog.setUniform("color", 0.f, 1.f, 1.f, .8f);


            if (drawTargetP2P)
                glDrawArrays(GL_POINTS, 0, (GLsizei)idxs.size());

            gpuX.bind();
            glVertexAttribPointer(0, dim, GLType<R>::val, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);  // Vertex position


            // make sure this is before the next draw
            if (actConstrainVertex >= 0) {
                prog.setUniform("color", 1.f, 0.f, 1.f, .8f);
                const int id = idxs[actConstrainVertex];
                glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, &id);
            }

            glPointSize(pointSize + 5);
            prog.setUniform("color", 0.f, 0.f, 0.f, .8f);
            glDrawElements(GL_POINTS, (GLsizei)idxs.size(), GL_UNSIGNED_INT, idxs.data());

            glPointSize(1.f);
        }

        prog.unbind();
    }

    int moveCurrentVertex(int x, int y, const int* vp)
    {
        if (actVertex < 0 || actVertex >= nVertex) return -1;

        auto mv = matMVP(vp);
        ConstMat4 MVP(mv.data());

        Eigen::Vector4f v; v[3] = 1;
        getVertex(actVertex, v.data());
        //mesh.X[actVertex*3]
        v = MVP*v;
        Eigen::Vector4f x1 = MVP.inverse().eval()*Eigen::Vector4f(x / R(vp[2]) * 2 - 1, 1 - y / R(vp[3]) * 2, v[2]/v[3], 1); // Make Sure call eval before topRows
        x1 = x1 / x1[3];

        //R v0[] = { mesh.X[pickVertex*dim], mesh.X[pickVertex*dim + 1], 0, 1 };
        //auto v1 = ConstMat4(mv.data()).transpose()*Eigen::Map<Eigen::Vector4f>(v0);

        //mesh.X[pickVertex*dim] = x1[0];
        //mesh.X[pickVertex*dim+1] = x1[1];
        //Eigen::Vector4f x2 = Eigen::Map<Eigen::Matrix4f>(mv.data()).transpose()*x1;

        setVertex(actVertex, x1.data());

        if (constrainVertices.find(actVertex) != constrainVertices.end()) 
            constrainVertices[actVertex] = { x1[0], x1[1], x1[2] };

        return 0;
    }

    int pick(int x, int y, const int *vp, int pickElem, int operation)
    {
        int idx = -1;

        if (idx < 0) {
            //ensure(nFace < 16777216, "not implemented for big mesh");

            //glDrawBuffer(GL_BACK);
            glDisable(GL_MULTISAMPLE); // make sure the color will not be interpolated

            glClearColor(1.f, 1.f, 1.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            pickProg.bind();
            pickProg.setUniform("MVP", matMVP(vp).data());
            pickProg.setUniform("pickElement", pickElem);  // 0/1 for pick vertex/face
            glBindVertexArray(vaoHandle);


            float pickdist = 12.f;
            glPointSize(pickdist);
            if (pickElem == PE_VERTEX) {
                // write depth for the whole shape
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDrawElements(GL_TRIANGLES, 3 * nFace, GL_UNSIGNED_INT, nullptr); // not GL_INT
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                pickProg.setUniform("MVP", matMVP(vp,true).data());
                glDrawArrays(GL_POINTS, 0, nVertex);
            }
            else
                glDrawElements(GL_TRIANGLES, 3 * nFace, GL_UNSIGNED_INT, nullptr); // not GL_INT

            unsigned char pixel[4];
            glReadPixels(x, vp[3] - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);   // y is inverted in OpenGL
            idx = (pixel[0] + pixel[1] * 256 + pixel[2] * 256 * 256) - 1;   // -1 to get true index, 0 means background

            glBindVertexArray(0);
            pickProg.unbind();
            //glDrawBuffer(GL_FRONT);
            glClearColor(1.f, 1.f, 1.f, 0.f);
        }


        if (pickElem == PE_VERTEX)
            actVertex = idx;
        else if (pickElem == PE_FACE)
            actFace = idx;

        int res = 0; // return how many vertex are added/deleted
        if (idx >= 0) {
            //printf("vertex %d is picked\n", idx);
            if (pickElem == PE_VERTEX) {
                auto it = constrainVertices.find(idx);
                if (it == constrainVertices.end()) {
                    if (operation == PO_ADD && idx < nVertex) {  // add constrain
                        vec v;
                        getVertex(idx, v.data());
                        constrainVertices[idx] = v;
                        res = 1;
                    }
                }
                else if (operation == PO_REMOVE) {
                    constrainVertices.erase(it);
                    res = -1;
                }
            }
        }

        auto i = getConstrainVertexIds();
        auto it = std::find(i.cbegin(), i.cend(), actVertex);
        actConstrainVertex = int((it == i.cend()) ? -1 : (it - i.cbegin()));
        return res;
    }

    static void boundingbox(const R* x, int nv, R *bbox)
    {
        if (nv < 1) {
            printf("empty point set!\n");
            return;
        }

        for (int i = 0; i < dim; i++) bbox[i] = bbox[i + dim] = x[i];

        for (int i = 1; i < nv; i++) {
            for (int j = 0; j < dim; j++) {
                bbox[j] = std::min(bbox[j], x[i*dim + j]);
                bbox[j + dim] = std::max(bbox[j + dim], x[i*dim + j]);
            }
        }
    }

    static void buildShaders() {
        const char* vertexShaderStr =
            R"( #version 330
    layout (location = 0) in vec3 VertexPosition;
    layout (location = 2) in vec2 VertexTexCoord;
    layout (location = 5) in float VertexData;
    out vec2 TexCoord;
    uniform mat4 MVP;

    uniform bool colorCoding = false;
    uniform vec2 dataMinMax = vec2(0, 1);

    void main(){
        gl_Position = MVP*vec4(VertexPosition, 1);
        if(colorCoding)
            TexCoord = vec2((VertexData-dataMinMax.x)/(dataMinMax.y-dataMinMax.x), 0.5);
        else
            TexCoord = VertexTexCoord;
    })";

        const char*  fragShaderStr =
            R"( #version 330
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D img;
    uniform float textureScale;
    uniform vec4 color;
    uniform bool useTexMap;
    void main(){
        FragColor = useTexMap?texture(img, textureScale*TexCoord):color;
        if(FragColor.a<1e-1)
            discard;  // important for transpancy with self overlapping
    })";

        prog.compileAndLinkAllShadersFromString(vertexShaderStr, fragShaderStr);
        prog.bind();
        prog.setUniform("img", 0);
        prog.setUniform("textureScale", 1.f);

        pointSetProg.compileAndLinkAllShadersFromString(
            R"( #version 330
    layout (location = 0) in vec3 VertexPosition;
    layout (location = 1) in float VertexData;
    uniform float scale;
    uniform mat4 MVP;
    void main(){
        gl_PointSize = VertexData*scale;
        gl_Position = MVP*vec4(VertexPosition, 1);
    })",
            R"( #version 330
    out vec4 FragColor;
    uniform vec4 color;
    void main(){ FragColor = color; })");


        //////////////////////////////////////////////////////////////////////////
        pickProg.compileAndLinkAllShadersFromString(
            R"( #version 330
    layout (location = 0) in vec3 VertexPosition;
    flat out int vertexId;
    uniform mat4 MVP;
    void main(){
        gl_Position = MVP*vec4(VertexPosition, 1);
        vertexId = gl_VertexID;
    })",
            R"( #version 330
    uniform int pickElement;
    flat in int vertexId;
    out vec4 FragColor;
    void main(){
        int id = ( (pickElement==0)?vertexId:gl_PrimitiveID ) + 1;
        // Convert the integer id into an RGB color
        FragColor = vec4( (id & 0x000000FF) >>  0, (id & 0x0000FF00) >>  8, (id & 0x00FF0000) >> 16, 255.f)/255.f;
    })");



const unsigned char jetmaprgb[] = {
0,  0,144,
  0,  0,160,
  0,  0,176,
  0,  0,192,
  0,  0,208,
  0,  0,224,
  0,  0,240,
  0,  0,255,
  0, 16,255,
  0, 32,255,
  0, 48,255,
  0, 64,255,
  0, 80,255,
  0, 96,255,
  0,112,255,
  0,128,255,
  0,144,255,
  0,160,255,
  0,176,255,
  0,192,255,
  0,208,255,
  0,224,255,
  0,240,255,
  0,255,255,
 16,255,240,
 32,255,224,
 48,255,208,
 64,255,192,
 80,255,176,
 96,255,160,
112,255,144,
128,255,128,
144,255,112,
160,255, 96,
176,255, 80,
192,255, 64,
208,255, 48,
224,255, 32,
240,255, 16,
255,255,  0,
255,240,  0,
255,224,  0,
255,208,  0,
255,192,  0,
255,176,  0,
255,160,  0,
255,144,  0,
255,128,  0,
255,112,  0,
255, 96,  0,
255, 80,  0,
255, 64,  0,
255, 48,  0,
255, 32,  0,
255, 16,  0,
255,  0,  0,
240,  0,  0,
224,  0,  0,
208,  0,  0,
192,  0,  0,
176,  0,  0,
160,  0,  0,
144,  0,  0,
128,  0,  0 };

        colormapTex.setImage(MyImage((BYTE*)jetmaprgb, sizeof(jetmaprgb) / 3, 1, sizeof(jetmaprgb), 3));  // bug fix, pitch should be w*3
        colormapTex.setClamping(GL_CLAMP_TO_EDGE);
    }
};


typedef GLMesh<> MyMesh;
