#ifndef ERER_CORE_SHADER_H_
#define ERER_CORE_SHADER_H_

#include "data_structure.hpp"
#include "image.h"
#include "../utils/math.h"

namespace Core
{
    struct Attribute
    {
        Matrix4f M;
        Matrix4f V;
        Matrix4f P;

        // For diffuse color cal
        Vector3f world_light_dir;
        Vector3f light_color; // range of color is 0-1(0-255)
        float light_intensity;

        // For specular color cal
        Vector3f camera_postion;
        Vector3f specular_color;

        // For ambient color
        Vector3f ambient;
    }; // system attributes
    struct Uniform
    {
        Image<Vector4c> albedo; // control the primary color of the surface
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
        Vector4f CS_POSITION; // clip space postion
        Vector3f WS_POSITION; // world space position
        Vector3f WS_NORMAL;   // world space normal
        Vector3f UV;
        Vector3f COLOR;
    }; // vertex stage outputs

    struct FragmentInput
    {
        Vector3f IWS_POSITION; // interpolation  world space postion
        Vector3f IWS_NORMAL;   // interpolation world space normal
        Vector3f I_UV;
        Vector3f I_COLOR; // interpolation color
    };                    // frag stage inputs

    // namespace GouraudShader
    // {
    //     VertexOutput vert(const VertexInput &vi, const Attribute &attribute, const Uniform &uniform)
    //     {
    //         VertexOutput vo;
    //         vo.CS_POSITION = attribute.P.mul(attribute.V.mul(attribute.M.mul(vi.MS_POSITION.reshape<4>(1))));
    //         // vo.COLOR0 = Vector3i{static_cast<int>(vi.TEXCOORD0[0] * 255), static_cast<int>(vi.TEXCOORD0[1] * 255), static_cast<int>(vi.TEXCOORD0[2] * 255)};

    //         Vector3f world_normal = attribute.M.mul(vi.MS_NORMAL.reshape<4>(0)).reshape<3>().normal();
    //         Vector3f world_half_dir = (-1 * attribute.world_light_dir - attribute.camera_postion).normal();
    //         Vector3f diffuse = attribute.light_color * uniform.albedo.sampling(vi.UV[0], vi.UV[1]) * attribute.light_intensity * std::max(0.f, Utils::dot_product_3D(world_normal, -1 * attribute.world_light_dir));
    //         Vector3f specular = attribute.light_color * attribute.specular_color * attribute.light_intensity * std::pow(std::max(0.f, Utils::dot_product_3D(world_normal, world_half_dir)), uniform.gloss);
    //         vo.COLOR = diffuse + attribute.ambient + specular;
    //         return vo;
    //     }

    //     Vector4i frag(FragmentInput fi, const Attribute &attribute, const Uniform &uniform)
    //     {
    //         return Vector4i{static_cast<int>(Utils::saturate(fi.I_COLOR[0]) * 255),
    //                         static_cast<int>(Utils::saturate(fi.I_COLOR[1]) * 255),
    //                         static_cast<int>(Utils::saturate(fi.I_COLOR[2]) * 255),
    //                         0};
    //     }
    // }

    namespace PhongShader
    {
        VertexOutput vert(const VertexInput &vi, const Attribute &attribute, const Uniform &uniform)
        {
            VertexOutput vo;
            vo.CS_POSITION = attribute.P.mul(attribute.V.mul(attribute.M.mul(vi.MS_POSITION.reshape<4>(1))));
            vo.WS_POSITION = attribute.M.mul(vi.MS_POSITION.reshape<4>(1)).reshape<3>();
            vo.WS_NORMAL = attribute.M.mul(vi.MS_NORMAL.reshape<4>(0)).reshape<3>().normal();
            vo.UV = vi.UV;

            return vo;
        }

        Vector4i frag(FragmentInput fi, const Attribute &attribute, const Uniform &uniform)
        {
            Vector3f world_half_dir = (attribute.camera_postion - fi.IWS_POSITION - attribute.world_light_dir).normal(); // light/world dir must reverse to keep the half vector on the same side with the normal vector
            Vector3f diffuse = attribute.light_color * Utils::tone_mapping(uniform.albedo.sampling(fi.I_UV[0], fi.I_UV[1])).reshape<3>() * attribute.light_intensity * std::max(0.f, Utils::dot_product_3D(fi.IWS_NORMAL, -1 * attribute.world_light_dir));
            Vector3f specular = attribute.light_color * attribute.specular_color * attribute.light_intensity * std::pow(std::max(0.f, Utils::dot_product_3D(fi.IWS_NORMAL, world_half_dir)), uniform.gloss);
            Vector3f tmp = diffuse + attribute.ambient + specular;
            return Vector4i{static_cast<int>(Utils::saturate(tmp[0]) * 255),
                            static_cast<int>(Utils::saturate(tmp[1]) * 255),
                            static_cast<int>(Utils::saturate(tmp[2]) * 255),
                            0};
            // return Vector4i{static_cast<int>(Utils::saturate(0) * 255),
            //                 static_cast<int>(Utils::saturate(0) * 255),
            //                 static_cast<int>(Utils::saturate(0) * 255),
            //                 0};
        }
    }

}

#endif // ERER_CORE_SHADER_H_
