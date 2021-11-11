#ifndef ERER_CORE_SHADER_H_
#define ERER_CORE_SHADER_H_

#include "data_structure.hpp"

namespace Core
{
    struct Attribute
    {
        Matrix4f M;
        Matrix4f V;
        Matrix4f P;
        Matrix4f ViewPort;
        Vector3f light_dir;
    }; // system attributes
    struct Uniform
    {
        // Pixmap
    }; // user self-defined

    struct VertexInput
    {
        Vector3f POSITION;
        Vector3f NORMAL;
        Vector3f TEXCOORD0;
    }; // vertex stage inputs

    struct VertexOutput
    {
        Vector3f POSITION;
        Vector3f NORMAL;
        Vector3i COLOR0;
    }; // vertex stage outputs

    struct FragmentInput
    {
        // Vector2i SV_POSITION;
        Vector3f SV_NORMAL;
        Vector3i COLOR0;
    }; // frag stage inputs

    VertexOutput vert(const VertexInput &i, const Attribute &attribute, const Uniform &uniform)
    {
        VertexOutput o;
        o.POSITION = attribute.ViewPort.mul(attribute.P.mul(attribute.V.mul(attribute.M.mul(i.POSITION.reshape<4>(0))))).reshape<3>();
        // o.POSITION = attribute.P.mul(attribute.V.mul(attribute.M.mul(i.POSITION.reshape<4>(0)))).reshape<3>();
        // std::cout << attribute.M;
        // std::cout << attribute.V;
        // std::cout << attribute.P;
        o.COLOR0 = Vector3i{255, 255, 255};
        return o;
    }

    Vector4i frag(FragmentInput i)
    {
        return Vector4i{i.COLOR0[0], i.COLOR0[1], i.COLOR0[2], 0};
    }

}

#endif // ERER_CORE_SHADER_H_
