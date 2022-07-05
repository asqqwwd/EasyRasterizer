#ifndef ERER_CORE_SHADER_H_
#define ERER_CORE_SHADER_H_

#include "data_structure.hpp"
#include "image.h"
#include "../utils/math.h"

namespace Core
{
    struct MeshAttribute
    {
        Matrix4f M;
        Image<Vector4c> albedo; // control the primary color of the surface
        float gloss;
    };

    struct LightAttribute
    {
        // For diffuse color cal
        Vector3f world_light_dir;
        Vector3f light_color; // range of color is 0-1(0-255)
        float light_intensity;

        // For specular color cal
        Vector3f specular_color;

        // For ambient color
        Vector3f ambient;
    };

    struct CameraAttribute
    {
        Matrix4f V;
        Matrix4f P;

        Vector3f camera_postion;
    };

    struct VertexInput
    {
        Vector4f MS_POSITION; // model space postion
        Vector3f MS_NORMAL;   // model space normal
        Vector3f UV;
    }; // vertex stage inputs

    struct VertexOutput
    {
        Vector4f CS_POSITION; // clip space postion
        Vector4f WS_POSITION; // world space position
        Vector3f WS_NORMAL;   // world space normal
        Vector3f UV;

        VertexOutput& operator+=(const VertexOutput& vert)
        {
            CS_POSITION += vert.CS_POSITION;
            WS_POSITION += vert.WS_POSITION;
            WS_NORMAL += vert.WS_NORMAL;
            UV += vert.UV;
            return *this;
        }
        VertexOutput operator+(const VertexOutput& vert) const
        {
            return VertexOutput(*this) += vert;
        }

        VertexOutput& operator*=(float value)
        {
            CS_POSITION *= value;
            WS_POSITION *= value;
            WS_NORMAL *= value;
            UV *= value;
            return *this;
        }
        VertexOutput operator*(float value) const
        {
            return VertexOutput(*this) *= value;
        }
        friend VertexOutput operator*(float value, const VertexOutput& vert)
        {
            return VertexOutput(vert) *= value;
        }
    }; // vertex stage outputs

    struct FragmentInput
    {
        Vector4f IWS_POSITION; // interpolation  world space postion
        Vector3f IWS_NORMAL;   // interpolation world space normal
        Vector3f I_UV;
    }; // frag stage inputs

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
        VertexOutput vert(const VertexInput& vi, const CameraAttribute& ca, const MeshAttribute& ma)
        {
            VertexOutput vo;
            vo.WS_POSITION = ma.M.mul(vi.MS_POSITION);
            vo.CS_POSITION = ca.P.mul(ca.V.mul(vo.WS_POSITION));
            // std::cout<<vo.CS_POSITION;
            vo.WS_NORMAL = ma.M.mul(vi.MS_NORMAL.reshape<4>(0)).reshape<3>().normal();
            vo.UV = vi.UV;

            return vo;
        }

        Vector4c frag(FragmentInput fi, const LightAttribute& la, const CameraAttribute& ca, const MeshAttribute& ma, float cover_rate)
        {
            Vector3f world_half_dir = (ca.camera_postion - fi.IWS_POSITION.reshape<3>() - la.world_light_dir).normal(); // light/world dir must reverse to keep the half vector on the same side with the normal vector
            Vector3f diffuse = la.light_color * Utils::tone_mapping(cover_rate * ma.albedo.sampling(fi.I_UV[0], 1 - fi.I_UV[1])).reshape<3>() * la.light_intensity * std::max(0.f, Utils::dot_product(fi.IWS_NORMAL, -1 * la.world_light_dir));
            Vector3f specular = la.light_color * la.specular_color * la.light_intensity * std::pow(std::max(0.f, Utils::dot_product(fi.IWS_NORMAL, world_half_dir)), ma.gloss);
            Vector3f tmp = diffuse + la.ambient + specular;
            // Vector3f tmp = diffuse; // Test
            return Vector4c{ static_cast<uint8_t>(Utils::saturate(tmp[0]) * 255),
                            static_cast<uint8_t>(Utils::saturate(tmp[1]) * 255),
                            static_cast<uint8_t>(Utils::saturate(tmp[2]) * 255),
                            0 };
            // return Vector4i{static_cast<int>(Utils::saturate(0) * 255),
            //                 static_cast<int>(Utils::saturate(0) * 255),
            //                 static_cast<int>(Utils::saturate(0) * 255),
            //                 0};
        }
    }

}

#endif // ERER_CORE_SHADER_H_
