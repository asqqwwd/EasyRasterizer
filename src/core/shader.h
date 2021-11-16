#ifndef ERER_CORE_SHADER_H_
#define ERER_CORE_SHADER_H_

#include "data_structure.hpp"
#include "tgaimage.h"

namespace Core
{
    struct Attribute
    {
        Matrix4f M;
        Matrix4f V;
        Matrix4f P;
        Matrix4f ViewPort;
        Vector3f light_dir;
        float light_intensity;
    }; // system attributes
    struct Uniform
    {
        TGAImage albedo; // control the primary color of the surface
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

    Vector3i sampling(const TGAImage &img, const Vector3f &uv)
    {
        TGAColor tmp = img.get(static_cast<int>(uv[0] * img.get_width()), static_cast<int>((1 - uv[1]) * img.get_height()));
        return Vector3i{tmp[2], tmp[1], tmp[0]}; // bgr->rgb
    }

    VertexOutput vert(const VertexInput &vi, const Attribute &attribute, const Uniform &uniform)
    {
        VertexOutput vo;
        Vector4f tmp = attribute.ViewPort.mul(attribute.P.mul(attribute.V.mul(attribute.M.mul(vi.POSITION.reshape<4>(1)))));
        tmp = tmp / tmp[3];
        vo.POSITION = tmp.reshape<3>();
        // vo.COLOR0 = Vector3i{static_cast<int>(vi.TEXCOORD0[0] * 255), static_cast<int>(vi.TEXCOORD0[1] * 255), static_cast<int>(vi.TEXCOORD0[2] * 255)};
        vo.COLOR0 = sampling(uniform.albedo, vi.TEXCOORD0);
        return vo;
    }

    Vector4i frag(FragmentInput i)
    {
        return Vector4i{i.COLOR0[0], i.COLOR0[1], i.COLOR0[2], 0};
    }

}

#endif // ERER_CORE_SHADER_H_
