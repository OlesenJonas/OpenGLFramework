#ifndef CAMERA_MATRICES_GLSL
#define CAMERA_MATRICES_GLSL

struct CameraMatrices
{
    mat4 View;
    mat4 invView;
    mat4 Proj;
    mat4 invProj;
    mat4 ProjView;
    mat4 invProjView;
};

#endif