#ifndef ERER_CORE_SHADER_H_
#define ERER_CORE_SHADER_H_

#include "data_structure.hpp"
#include "tgaimage.h"
#include "../utils/math.h"

namespace Core
{
    struct Attribute
    {
        Matrix4f M;
        Matrix4f V;
        Matrix4f P;
        Matrix4f ViewPort;

        // For diffuse color cal
        Vector3f world_light_dir;
        Vector3f light_color; // range of color is 0-1(0-255)
        float light_intensity;

        // For specular color cal
        Vector3f world_view_dir;
        Vector3f specular_color;

        // For ambient color
        Vector3f ambient;
    }; // system attributes
    struct Uniform
    {
        TGAImage albedo; // control the primary color of the surface
        float gloss;
        // Pixmap
    }; // user self-defined

    struct VertexInput
    {
        Vector3f MS_POSITION; // model space postion
        Vector3f MS_NORMAL;   // model space normal
        Vector3f UV;
    }; // vertex stage inputs

    struct VertexOutput
    {
        Vector3f SS_POSITION; // screen space postion
        Vector3f WS_NORMAL;   // world space normal
        Vector3f UV;
        Vector3f COLOR;
    }; // vertex stage outputs

    struct FragmentInput
    {
        // Vector3f ISS_POSITION; // interpolation  screen space postion
        Vector3f IWS_NORMAL; // interpolation world space normal
        Vector3f I_UV;
        Vector3f I_COLOR; // interpolation color
    };                    // frag stage inputs

    Vector3f sampling(const TGAImage &img, const Vector3f &uv)
    {
        TGAColor tmp = img.get(static_cast<int>(uv[0] * img.get_width() + 0.5), static_cast<int>((1 - uv[1]) * img.get_height() + 0.5));
        return Vector3f{tmp[0] / 255.f, tmp[1] / 255.f, tmp[2] / 255.f}; // bgr->bgr
    }

    namespace GouraudShader
    {
        VertexOutput vert(const VertexInput &vi, const Attribute &attribute, const Uniform &uniform)
        {
            VertexOutput vo;
            Vector4f tmp = attribute.ViewPort.mul(attribute.P.mul(attribute.V.mul(attribute.M.mul(vi.MS_POSITION.reshape<4>(1)))));
            tmp = tmp / tmp[3];
            vo.SS_POSITION = tmp.reshape<3>();
            // vo.COLOR0 = Vector3i{static_cast<int>(vi.TEXCOORD0[0] * 255), static_cast<int>(vi.TEXCOORD0[1] * 255), static_cast<int>(vi.TEXCOORD0[2] * 255)};

            Vector3f world_normal = attribute.M.mul(vi.MS_NORMAL.reshape<4>(0)).reshape<3>().normal();
            Vector3f world_half_dir = (attribute.world_light_dir + attribute.world_view_dir).normal();
            Vector3f diffuse = attribute.light_color * sampling(uniform.albedo, vi.UV) * attribute.light_intensity * std::max(0.f, Utils::dot_product_3D(world_normal, attribute.world_light_dir));
            Vector3f specular = attribute.light_color * attribute.specular_color * attribute.light_intensity * std::pow(std::max(0.f, Utils::dot_product_3D(world_normal, world_half_dir)), uniform.gloss);
            vo.COLOR = diffuse + attribute.ambient + specular;
            return vo;
        }

        Vector4i frag(FragmentInput fi, const Attribute &attribute, const Uniform &uniform)
        {
            return Vector4i{static_cast<int>(Utils::saturate(fi.I_COLOR[0]) * 255),
                            static_cast<int>(Utils::saturate(fi.I_COLOR[1]) * 255),
                            static_cast<int>(Utils::saturate(fi.I_COLOR[2]) * 255),
                            0};
        }
    }

    namespace PhongShader
    {
        VertexOutput vert(const VertexInput &vi, const Attribute &attribute, const Uniform &uniform)
        {
            VertexOutput vo;
            Vector4f tmp = attribute.ViewPort.mul(attribute.P.mul(attribute.V.mul(attribute.M.mul(vi.MS_POSITION.reshape<4>(1)))));
            tmp = tmp / tmp[3];
            vo.SS_POSITION = tmp.reshape<3>();
            vo.WS_NORMAL = attribute.M.mul(vi.MS_NORMAL.reshape<4>(0)).reshape<3>().normal();
            vo.UV = vi.UV;

            return vo;
        }

        Vector4i frag(FragmentInput fi, const Attribute &attribute, const Uniform &uniform)
        {
            Vector3f world_half_dir = (attribute.world_light_dir + attribute.world_view_dir).normal();
            Vector3f diffuse = attribute.light_color * sampling(uniform.albedo, fi.I_UV) * attribute.light_intensity * std::max(0.f, Utils::dot_product_3D(fi.IWS_NORMAL, attribute.world_light_dir));
            Vector3f specular = attribute.light_color * attribute.specular_color * attribute.light_intensity * std::pow(std::max(0.f, Utils::dot_product_3D(fi.IWS_NORMAL, world_half_dir)), uniform.gloss);
            Vector3f tmp = diffuse + attribute.ambient + specular;
            return Vector4i{static_cast<int>(Utils::saturate(tmp[0]) * 255),
                            static_cast<int>(Utils::saturate(tmp[1]) * 255),
                            static_cast<int>(Utils::saturate(tmp[2]) * 255),
                            0};
        }
    }

}

#endif // ERER_CORE_SHADER_H_
