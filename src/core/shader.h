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
    };
    struct Uniform
    {
        // Pixmap
    };

    struct a2v
    {
        Vector3f POSITION;
        Vector3f NORMAL;
        Vector2f TEXCOORD0;
    };

    struct v2f
    {
        Vector3f SV_POSITION;
        Vector4i COLOR0;
    };

    v2f vert(const a2v &v, const Attribute &attribute, const Uniform &uniform)
    {
        v2f o;
        o.SV_POSITION = attribute.ViewPort.mul(attribute.P.mul(attribute.V.mul(attribute.M.mul(v.POSITION.reshape<4>(0))))).reshape<3>();
        return o;
    }

    Vector4i frag(v2f i)
    {
        return Vector4i{0, 0, 0, 0};
    }

}

#endif // ERER_CORE_SHADER_H_
