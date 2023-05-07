//   Copyright © 2021, Renjie Chen @ USTC

#pragma once
#include <algorithm>
#include <vector>
#include <array>

#include "glprogram.h"
#include "glarray.h"


template<typename R = float, int dimension = 3>
struct GLMesh
{
    enum { dim = dimension };

    struct Mesh
    {
        std::vector<R> X, UV;
        std::vector<int> T;
        size_t nVertex() const { return X.size() / dim; }
        size_t nFace() const { return T.size() / 3; }
    };
    Mesh mesh;
    GLTexture tex;

    typedef std::array<R, 4> vec4;
    typedef std::array<R, dim> vec;

    int nVertex;
    int nFace;
    GLuint vaoHandle;

    GLArray<R, dim> gpuX;
    GLArray<int, 3, true> gpuT;
    GLArray<R, 2>   gpuUV;

    float mMeshScale = 1.f;

    vec mTranslate = { 0, 0, 0 };
    R bbox[dim * 2];

    bool drawTargetP2P = true;

    static GLProgram prog;

    ~GLMesh() { glDeleteVertexArrays(1, &vaoHandle); }

    GLMesh() :vaoHandle(0) {}

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

        gpuT.allocateStorage(nf);

        glBindVertexArray(0);
    }


    void upload(const R *x, int nv, const int *t, int nf, const R *uv)
    {
        allocateStorage(x ? nv : nVertex, t ? nf : nFace);

        if (x) { gpuX.uploadData(x, nv, false); mesh.X.assign(x, x + nv*dim); }
        if (uv) { gpuUV.uploadData(uv, nv, false); mesh.UV.assign(uv, uv + nv * 2); }
        if (t) { gpuT.uploadData(t, nf, false); mesh.T.assign(t, t + nf * 3); }

        if(x&&t)
            boundingbox(x, nv, bbox);  // update bounding box for the initialization
    }

    void updateBBox() {
        boundingbox(mesh.X.data(), nVertex, bbox);
    }

    R drawscale() const
    {
        R scale0 = 1.9f / std::max(std::max(bbox[dim] - bbox[0], bbox[1 + dim] - bbox[1]), bbox[2 + dim] - bbox[2]);
        return mMeshScale*scale0;
    }


    std::array<R, 16> modelView(const int *vp, bool colmajor=false) const{
        R vscaling = vp[2] / R(vp[3]);
        R trans[] = { (bbox[0] + bbox[dim]) / 2, (bbox[1] + bbox[1+dim]) / 2 };
        R xform[2] = { 1, 1 };
        if (vscaling < 1)
            xform[1] = vscaling;
        else
            xform[0] = 1 / vscaling;

        R ss = drawscale();
        R s[2] = {ss*xform[0], ss*xform[1]};
        R t[] = { mTranslate[0] - trans[0], mTranslate[1] - trans[1] };
        return colmajor?std::array<R, 16>{
            s[0], 0, 0, 0, 
            0, s[1], 0, 0,
            0, 0, 1, 0,
            t[0] * s[0], t[1] * s[1], 0, 1 }
            :std::array<R, 16>{
            s[0], 0, 0, t[0] * s[0],
            0, s[1], 0, t[1] * s[1],
            0, 0, 1, 0,
            0, 0, 0, 1 };
    }

    void moveInScreen(int x0, int y0, int x1, int y1, int *vp) {
        float dx = (x1 - x0) * 2.f / vp[2];
        float dy = -(y1 - y0) * 2.f / vp[3];

       float ss = drawscale();
       mTranslate[0] += dx / ss;
       mTranslate[1] += dy / ss;
    }


    void draw(const int *vp)
    {
        glActiveTexture(GL_TEXTURE0);
        tex.bind();

        prog.bind();
        prog.setUniform("MVP", modelView(vp).data());

        glBindVertexArray(vaoHandle);
        glDrawElements(GL_TRIANGLES, 3 * nFace, GL_UNSIGNED_INT, nullptr); // not GL_INT
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
    out vec2 TexCoord;
    uniform mat4 MVP;

    void main(){
        gl_Position = MVP*vec4(VertexPosition, 1);
        TexCoord = VertexTexCoord;
    })";

        const char*  fragShaderStr =
            R"( #version 330
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D img;
    void main(){
        FragColor = texture(img, TexCoord);
    })";

        prog.compileAndLinkAllShadersFromString(vertexShaderStr, fragShaderStr);
        prog.bind();
        prog.setUniform("img", 0);
    }
};


typedef GLMesh<> MyMesh;
