#include "RQShader.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>

#include "../vendor/armhook/patch.h"
#include "Streaming.h"
#include "Water/CWaterFX.h"
#include "TimeCycle.h"
#include "Scene.h"
#include "../graphics/CSkyBox.h"

ShaderCreator vertexSrc;
ShaderCreator pixelSrc;

static const char *WATER_PIXEL_REFRACTION =
"#version 100\n"
"precision highp float;\n"
"uniform sampler2D Diffuse;\n"
"uniform sampler2D EnvMap;\n"
"uniform sampler2D RefractionMap;\n"
"uniform sampler2D FogMap;\n"
"uniform vec3 FogDistances;\n"
"varying vec2 Out_Tex0;\n"
"varying vec4 Out_ClipSpace;\n"
"varying vec3 Out_CameraVector;\n"
"varying vec2 Out_DuDvTexCoord;\n"
"varying vec3 Out_Refl;\n"
"varying mediump float Out_FogAmt;\n"
"void main()\n"
"{\n"
"	vec2 ndc = (Out_ClipSpace.xy/Out_ClipSpace.w) / 2.0 + 0.5;\n"
"\n"
"	float Waveness = FogDistances.y;\n"
"	vec2 duv1 = texture2D(Diffuse, Out_DuDvTexCoord + vec2(Waveness, 0.0)).rg;\n"
"	vec2 duv2 = texture2D(Diffuse, Out_DuDvTexCoord * 1.7 - vec2(0.0, Waveness * 0.8)).rg;\n"
"	vec2 distSum = ((duv1 + duv2) - 1.0) * 0.02;\n"
"\n"
"	vec2 refractTexCoords = vec2(ndc.x, ndc.y);\n"
"	refractTexCoords += distSum;\n"
"	refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);\n"
"\n"
"	vec2 ReflPos = vec2(ndc.x, ndc.y);\n"
"	ReflPos += distSum;\n"
"\n"
"	ReflPos = clamp(ReflPos, 0.001, 0.999); \n"
"\n"
"	vec4 reflectTex = texture2D(EnvMap, ReflPos); \n"
"\n"
"	vec4 refract = texture2D(RefractionMap, refractTexCoords);\n"
"	vec3 ViewVector = normalize(Out_CameraVector);\n"
"	float RefractionCoef = dot(ViewVector, vec3(0.0, 0.0, 1.0));\n"
"	float fresnel = pow(1.0 - clamp(RefractionCoef, 0.0, 1.0), 3.0);\n"
"	RefractionCoef = clamp(1.0 - fresnel, 0.08, 0.92);\n"
"	\n"
"	gl_FragColor = mix(reflectTex, refract, RefractionCoef);\n"
"	gl_FragColor = mix(gl_FragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2);\n"
"	lowp vec4 fogColorNew = texture2D(FogMap, ndc);\n"
"	gl_FragColor.xyz = mix(gl_FragColor.xyz, fogColorNew.xyz, Out_FogAmt);\n"
"}\n";

static const char *WATER_VERTEX_REFRACTION =
"#version 100\n"
"precision highp float;\n"
"uniform mat4 ProjMatrix;\n"
"uniform mat4 ViewMatrix;\n"
"uniform mat4 ObjMatrix;\n"
"uniform vec3 CameraPosition;\n"
"varying mediump float Out_FogAmt;\n"
"uniform vec3 FogDistances;\n"
"attribute vec3 Position;\n"
"attribute vec2 TexCoord0;\n"
"varying vec2 Out_Tex0;\n"
"varying vec4 Out_ClipSpace;\n"
"varying vec3 Out_CameraVector;\n"
"varying vec2 Out_DuDvTexCoord;\n"
"varying vec3 Out_Refl;\n"
"void main()\n"
"{\n"
"	Out_Tex0 = TexCoord0;\n"
"	vec4 WorldPos = ObjMatrix * vec4(Position, 1.0);\n"
"	vec4 ViewPos = ViewMatrix * WorldPos;\n"
"	Out_ClipSpace = ProjMatrix * ViewPos;\n"
"	Out_CameraVector = CameraPosition - WorldPos.xyz;\n"
"	Out_DuDvTexCoord = vec2(Position.x/2.0 + 0.5, Position.y/2.0 + 0.5) * 0.2;\n"
"	\n"
"	vec3 WorldNormal = vec3(0.0, 0.0, 1.0);\n"
"	vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);\n"
"	reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;\n"
"	Out_Refl = reflVector;\n"
"	float fogDist = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 1.0);\n"
"	float fogHeight = clamp((165.0 - WorldPos.z) / 155.0, 0.0, 1.0);\n"
"	Out_FogAmt = fogDist * pow(fogHeight, 1.6);\n"
"	\n"
"	gl_Position = Out_ClipSpace;\n"
"}\n";

static const char *WATER_PIXEL_LITE =
"#version 100\n"
"precision highp float;\n"
"uniform sampler2D Diffuse;\n"
"uniform sampler2D EnvMap;\n"
"uniform vec3 FogDistances;\n"
"uniform lowp vec3 FogColor;\n"
"varying mediump vec2 Out_Tex0;\n"
"varying mediump vec4 Out_ClipSpace;\n"
"varying vec3 Out_CameraVector;\n"
"varying highp vec2 Out_DuDvTexCoord;\n"
"varying mediump vec3 Out_Refl;\n"
"varying mediump float Out_FogAmt;\n"
"varying lowp vec4 Out_Color;\n"
"void main()\n"
"{\n"
"	float WavePacked = FogDistances.y;\n"
"	float ReflSteps = max(floor(WavePacked), 0.0);\n"
"	float Waveness = WavePacked - ReflSteps;\n"
"	float ReflBias = ReflSteps * 0.01 - 0.5;\n"
"	mediump vec2 duv1 = texture2D(Diffuse, Out_DuDvTexCoord + vec2(Waveness, 0.0)).rg;\n"
"	mediump vec2 duv2 = texture2D(Diffuse, Out_DuDvTexCoord * 1.7 - vec2(0.0, Waveness * 0.8)).rg;\n"
"	mediump vec2 distSum = ((duv1 + duv2) - 1.0) * 0.012;\n"
"\n"
"	vec3 ToCam = normalize(Out_CameraVector);\n"
"	vec3 Refl = vec3(-ToCam.x, -ToCam.y, ToCam.z);\n"
"	float ReflLen = max(length(Refl.xy), 0.00001);\n"
"	mediump vec2 ReflPos = (Refl.xy / ReflLen) * (Refl.z * 0.5 + 0.5);\n"
"	ReflPos = ReflPos * 0.5 + 0.5;\n"
"	ReflPos.y += ReflBias;\n"
"	ReflPos += distSum;\n"
"\n"
"	ReflPos = clamp(ReflPos, 0.001, 0.999); \n"
"\n"
"	lowp vec4 reflectTex = texture2D(EnvMap, ReflPos); \n"
"\n"
"	vec3 ViewVector = normalize(Out_CameraVector);\n"
"	float fresnel = pow(1.0 - max(dot(ViewVector, vec3(0.0, 0.0, 1.0)), 0.0), 4.0);\n"
"\n"
"	mediump float reflFade = 1.0 - clamp(Out_FogAmt * 2.0, 0.0, 1.0);\n"
"\n"
"	mediump float reflAmt = clamp(0.35 + fresnel * 0.65, 0.0, 1.0) * reflFade;\n"
"	lowp vec3 color = mix(vec3(0.0, 0.3, 0.5), reflectTex.xyz, reflAmt);\n"
"	color = mix(color, FogColor, Out_FogAmt);\n"
"\n"
"	lowp vec4 plain = texture2D(Diffuse, Out_Tex0) * Out_Color;\n"
"	plain.xyz = mix(plain.xyz, FogColor, Out_FogAmt);\n"
"\n"
"	if (WavePacked < 0.0) {\n"
"		gl_FragColor = plain;\n"
"	} else {\n"
"		gl_FragColor = vec4(color, mix(1.0 - fresnel * 0.5, 1.0, Out_FogAmt));\n"
"	}\n"
"}\n";

static const char *WATER_VERTEX_LITE =
"#version 100\n"
"precision highp float;\n"
"uniform mat4 ProjMatrix;\n"
"uniform mat4 ViewMatrix;\n"
"uniform mat4 ObjMatrix;\n"
"uniform vec3 CameraPosition;\n"
"varying mediump float Out_FogAmt;\n"
"uniform vec3 FogDistances;\n"
"attribute vec3 Position;\n"
"attribute vec2 TexCoord0;\n"
"attribute vec4 GlobalColor;\n"
"varying mediump vec2 Out_Tex0;\n"
"varying mediump vec4 Out_ClipSpace;\n"
"varying vec3 Out_CameraVector;\n"
"varying highp vec2 Out_DuDvTexCoord;\n"
"varying mediump vec3 Out_Refl;\n"
"varying lowp vec4 Out_Color;\n"
"void main()\n"
"{\n"
"	Out_Tex0 = TexCoord0;\n"
"	Out_Color = GlobalColor;\n"
"	vec4 WorldPos = ObjMatrix * vec4(Position, 1.0);\n"
"	vec4 ViewPos = ViewMatrix * WorldPos;\n"
"	Out_ClipSpace = ProjMatrix * ViewPos;\n"
"	Out_CameraVector = CameraPosition - WorldPos.xyz;\n"
"	Out_DuDvTexCoord = vec2(Position.x/2.0 + 0.5, Position.y/2.0 + 0.5) * 0.2;\n"
"	\n"
"	vec3 WorldNormal = vec3(0.0, 0.0, 1.0);\n"
"	vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);\n"
"	reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;\n"
"	Out_Refl = reflVector;\n"
"	float fogDist = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 1.0);\n"
"	float fogHeight = clamp((165.0 - WorldPos.z) / 155.0, 0.0, 1.0);\n"
"	Out_FogAmt = fogDist * pow(fogHeight, 1.6);\n"
"	\n"
"	gl_Position = Out_ClipSpace;\n"
"}\n";

static std::string GlslFloat(float value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4f", value);
    return buf;
}

bool RQShader::BuildSource(uint32 flags, const char **pixelSource, const char **vertexSource)
{
    vertexSrc.shaderSrc[0] = 0;
    pixelSrc.shaderSrc[0] = 0;

    if ((flags & FLAG_WATER) && CWaterFX::ms_bEnabled) {
        strcpy(pixelSrc.shaderSrc, WATER_PIXEL_LITE);
        strcpy(vertexSrc.shaderSrc, WATER_VERTEX_LITE);

        *pixelSource = (const char*)&pixelSrc;
        *vertexSource = (const char*)&vertexSrc;
        return true;
    }

    BuildPixelSource(flags);
    BuildVertexSource(flags);

    *pixelSource = (const char*)&pixelSrc;
    *vertexSource = (const char*)&vertexSrc;
    return true;
}

static void (*ES2Shader__Select)(void *thiz);
static void ES2Shader__Select_hook(void *thiz)
{
    ES2Shader__Select(thiz);
    CWaterFX::OnShaderSelected(thiz);
}

static constexpr int SCV_FOG_DISTANCES = 13;
static constexpr int SCV_FOG_COLOR     = 14;

static void (*ES2Shader__SetVectorConstant)(void *thiz, int id, const float *values, int count);
static void ES2Shader__SetVectorConstant_hook(void *thiz, int id, const float *values, int count)
{
    if (id == SCV_FOG_DISTANCES && values && count >= 3 && RQShader::g_fFogEnd > 0.0f) {
        float start = RQShader::g_fFogStart;
        float end   = std::max(RQShader::g_fFogEnd, start + 1.0f);

        if (Scene.m_pRwCamera) {
            const float farPlane = Scene.m_pRwCamera->farPlane;
            if (farPlane > 1.0f && end > farPlane * 0.95f) {
                const float k = (farPlane * 0.95f) / end;
                end   *= k;
                start *= k;
            }
        }

        const int centerSteps = std::clamp(
                (int)lroundf(RQShader::g_fReflCenterY),
                0, (int)RQShader::REFL_CENTER_MAX);

        const float wavePhase = CWaterFX::ms_bEnabled
                ? CWaterFX::ms_fWaveness + (float)centerSteps
                : -1.0f;
        float custom[4] = { start, wavePhase, 1.0f / (end - start), 0.0f };
        if (count > 3) {
            custom[3] = values[3];
        }

        ES2Shader__SetVectorConstant(thiz, id, custom, count);
        return;
    }

    if (id == SCV_FOG_COLOR && values && count >= 3 && RQShader::g_bFogFromSky) {
        CRGBA horizon;
        float r, g, b;

        if (CSkyBox::GetFogColour(horizon)) {
            r = (float)horizon.r / 255.0f;
            g = (float)horizon.g / 255.0f;
            b = (float)horizon.b / 255.0f;
        } else {
            const auto &sky = CTimeCycle::m_CurrentColours;
            r = (float)sky.m_nSkyBottomRed   / 255.0f;
            g = (float)sky.m_nSkyBottomGreen / 255.0f;
            b = (float)sky.m_nSkyBottomBlue  / 255.0f;
        }

        float custom[4] = { r, g, b, count > 3 ? values[3] : 0.0f };

        ES2Shader__SetVectorConstant(thiz, id, custom, count);
        return;
    }

    ES2Shader__SetVectorConstant(thiz, id, values, count);
}

void RQShader::InjectHooks() {
    CHook::InlineHook("_ZN9ES2Shader17SetVectorConstantE24RQShaderVectorConstantIDPKfi",
                      &ES2Shader__SetVectorConstant_hook, &ES2Shader__SetVectorConstant);

    CHook::Redirect("_ZN8RQShader11BuildSourceEjPPKcS2_", &BuildSource);
    CHook::InlineHook("_ZN9ES2Shader6SelectEv", &ES2Shader__Select_hook, &ES2Shader__Select);
}

int OS_SystemChip(){
    return CHook::CallFunction<int>("_Z13OS_SystemChipv");
}

void BuildPixelSource(uint32 flags) {
    RQCapabilities* RQCaps = (RQCapabilities*)(g_libGTASA + (VER_x32 ? 0x6B8B9C : 0x896130));
    int ped_spec = 0;
    std::ostringstream buff;

    const bool screenFog = false;

    buff << "#version 100\n"
         << "precision mediump float;";

    if (flags & FLAG_TEX0) {
        buff << "uniform sampler2D Diffuse;"
             << "varying mediump vec2 Out_Tex0;";
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_WATER)) {
        if (flags & FLAG_ENVMAP)
            buff << "varying mediump vec2 Out_Tex1;";
        else
            buff << "varying mediump vec3 Out_Refl;";
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_DETAILMAP | FLAG_WATER)) {
        buff << "uniform sampler2D EnvMap;";
    }
    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
        buff << "uniform lowp float EnvMapCoefficient;";
    } else if (flags & FLAG_DETAILMAP) {
        buff << "uniform float DetailTiling;";
    }

    if (flags & FLAG_FOG) {
        buff << "varying mediump float Out_FogAmt;"
             << "uniform lowp vec3 FogColor;";

        if (screenFog) {
            buff << "uniform sampler2D FogMap;"
                 << "varying lowp vec4 Out_ClipSpace;";
        }
    }
    if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
        buff << "varying lowp vec4 Out_Color;";
    }
    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))) {
        buff << "varying lowp vec3 Out_Spec;";
    }
    if (flags & FLAG_ALPHA_MODULATE) {
        buff << "uniform lowp float AlphaModulate;";
    }
    if (flags & FLAG_WATER) {
        buff << "varying mediump vec2 Out_WaterDetail;"
             << "varying mediump vec2 Out_WaterDetail2;"
             << "varying mediump float Out_WaterAlphaBlend;";
    }

    buff << "void main() {"
         << "lowp vec4 fcolor;";

    if ((flags & FLAG_FOG) && screenFog) {
        buff << "lowp vec2 ndc = (Out_ClipSpace.xy/Out_ClipSpace.w) / 2.0 + 0.5;"
             << "lowp vec2 fogTexCoords = vec2(ndc.x, ndc.y);"
             << "lowp vec4 fogColorNew = texture2D(FogMap, fogTexCoords);";
    }

    if (!(flags & FLAG_TEX0)) {
        if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
            buff << "fcolor = Out_Color;";
        } else {
            buff << "fcolor = vec4(0.0, 0.0, 0.0, 0.0);";
        }
    } else {
        if (flags & FLAG_TEXBIAS) {
            buff << "lowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -1.5);";
        } else {
            buff << "lowp vec4 diffuseColor = texture2D(Diffuse, Out_Tex0, -0.5);";
        }
        buff << "fcolor = diffuseColor;";
        if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
            if (!(flags & FLAG_DETAILMAP)) {
                buff << "fcolor *= Out_Color;";
            } else if (flags & FLAG_WATER) {
                buff << "float waterDetail = texture2D(EnvMap, Out_WaterDetail, -1.0).x + texture2D(EnvMap, Out_WaterDetail2, -1.0).x;"
                     << "fcolor *= vec4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);"
                     << "fcolor.a += Out_WaterAlphaBlend + " << GlslFloat(RQShader::g_fWaterOpacity) << ";";
            } else {
                buff << "fcolor *= vec4(Out_Color.xyz * texture2D(EnvMap, Out_Tex0.xy * DetailTiling, -0.5).xyz * 2.0, Out_Color.w);";
            }
        }
    }

    if (flags & FLAG_ENVMAP) {
        buff << "fcolor.xyz = mix(fcolor.xyz, texture2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);";
    }

    if (flags & (FLAG_SPHERE_ENVMAP | FLAG_WATER)) {
        buff << "lowp vec4 ReflTexture;";

        if (flags & FLAG_WATER) {
             buff << "vec2 ReflPos = Out_Refl.xy;"
                  << "ReflPos = ReflPos * 0.5 + 0.5;"
                  << "ReflPos = clamp(ReflPos, 0.0, 1.0);"
                  << "ReflTexture = texture2D(EnvMap, ReflPos);";
        } else {
            buff  << "vec2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);"
                  << "ReflPos = (ReflPos * vec2(0.5,0.5)) + vec2(0.5,0.5);"
                  << "ReflTexture = texture2D(EnvMap, ReflPos);";
        }

        if (flags & FLAG_WATER) {
             buff << "float reflectFactor = 1.0 - smoothstep(24.0, 48.0, Out_WaterAlphaBlend);"
                  << "float reflectAmt = reflectFactor * 0.8;"
                  << "fcolor.xyz = mix(fcolor.xyz, ReflTexture.xyz, reflectAmt);";

             if (flags & FLAG_FOG) {
                if (screenFog) {
                    buff << "fcolor.xyz = mix(fcolor.xyz, fogColorNew.xyz, Out_FogAmt);";
                } else {
                    buff << "fcolor.xyz = mix(fcolor.xyz, FogColor, Out_FogAmt);";
                }
             }
        } else if (flags & FLAG_SPHERE_ENVMAP) {
             if (screenFog) {
                 buff << "float newEnvMapCoef = EnvMapCoefficient + 0.14;"
                      << "fcolor.xyz = mix(fcolor.xyz, ReflTexture.xyz, newEnvMapCoef);";
             } else {
                 buff << "fcolor.xyz = mix(fcolor.xyz, ReflTexture.xyz, EnvMapCoefficient);";
             }
             buff << "fcolor.w += ReflTexture.b * 0.125;";
        }
    }

    if (!RQCaps->unk_08) {
        if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))) {
            buff << "fcolor.xyz += Out_Spec;";
        }

        if (flags & FLAG_FOG) {
            if (!(flags & FLAG_WATER)) {
                if (screenFog) {
                    buff << "fcolor.xyz = mix(fcolor.xyz, fogColorNew.xyz, Out_FogAmt);";
                } else {
                    buff << "fcolor.xyz = mix(fcolor.xyz, FogColor, Out_FogAmt);";
                }
            }
        }
    }

    if (screenFog) {
        buff << "fcolor.xyz += fcolor.xyz * 0.5;";
    }

    if (flags & FLAG_GAMMA) {
        buff << "fcolor.xyz = pow(fcolor.xyz, vec3(1.0 / 2.2));";
    }

    buff << "gl_FragColor = fcolor;";

    if (flags & FLAG_ALPHA_TEST) {
        buff << "/*ATBEGIN*/";
        if (screenFog && (flags & FLAG_TEX0)) {
            if (flags & FLAG_TEXBIAS) {
                buff << "if (diffuseColor.a < 0.8) { discard; }";
            } else if (flags & FLAG_CAMERA_BASED_NORMALS) {
                buff << "if (diffuseColor.a < 0.5) { discard; }"
                     << "gl_FragColor.a = Out_Color.a;";
            } else {
                buff << "if (diffuseColor.a < 0.2) { discard; }";
            }
        } else if (flags & FLAG_TEXBIAS) {
            buff << "if (gl_FragColor.a < 0.8) { discard; }";
        } else if (flags & FLAG_CAMERA_BASED_NORMALS) {
            buff << "if (gl_FragColor.a < 0.5) { discard; }"
                 << "gl_FragColor.a = Out_Color.a;";
        } else {
            buff << "if (gl_FragColor.a < 0.2) { discard; }";
        }
        buff << "/*ATEND*/";
    }

    if (flags & FLAG_ALPHA_MODULATE) {
        buff << "gl_FragColor.a *= AlphaModulate;";
    }

    buff << "}";

    strcpy(pixelSrc.shaderSrc, buff.str().c_str());
}

int GetMobileEffectSetting() {
    return CHook::CallFunction<int>("_Z22GetMobileEffectSettingv");
}

void BuildVertexSource(uint32 flags)
{
    int& RQMaxBones = *(int(*))(g_libGTASA + (VER_x32 ? 0x6B8BAC : 0x896140));
    RQCapabilities* RQCaps = (RQCapabilities*)(g_libGTASA + (VER_x32 ? 0x6B8B9C : 0x896130));

    const bool screenFog = false;

    char* vertexColor;

    int ped_spec = 0;
    std::ostringstream buff;

    buff    << "#version 100\n"
            << "precision highp float;\n"
            << "uniform mat4 ProjMatrix;\n"
            << "uniform mat4 ViewMatrix;\n"
            << "uniform mat4 ObjMatrix;\n";

    if (flags & FLAG_LIGHTING)
    {
        buff    << "uniform lowp vec3 AmbientLightColor;\n"
                << "uniform lowp vec4 MaterialEmissive;\n"
                << "uniform lowp vec4 MaterialAmbient;\n"
                << "uniform lowp vec4 MaterialDiffuse;\n";

        if (flags & FLAG_LIGHT1)
        {
            buff    << "uniform lowp vec3 DirLightDiffuseColor;\n"
                    << "uniform vec3 DirLightDirection;\n";

            if (GetMobileEffectSetting() == 3) {
                if (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4))
                    buff << "uniform vec3 DirBackLightDirection;";
            }
        }
        if (flags & FLAG_LIGHT2)
        {
            buff    << "uniform lowp vec3 DirLight2DiffuseColor;\n"
                    << "uniform vec3 DirLight2Direction;\n";
        }
        if (flags & FLAG_LIGHT3)
        {
            buff    << "uniform lowp vec3 DirLight3DiffuseColor;\n"
                    << "uniform vec3 DirLight3Direction;\n";
        }
    }

    buff    << "attribute vec3 Position;\n"
            << "attribute vec3 Normal;\n";

    if (flags & FLAG_TEX0) {
        if (flags & FLAG_PROJECT_TEXCOORD)
            buff << "attribute vec4 TexCoord0;\n";
        else
            buff << "attribute vec2 TexCoord0;\n";
    }

    buff << "attribute vec4 GlobalColor;\n";

    if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
        buff    << "attribute vec4 BoneWeight;\n"
                << "attribute vec4 BoneIndices;\n"
                << "uniform highp vec4 Bones[" << 3 * RQMaxBones << "];\n";
    }

    if (flags & FLAG_TEX0)
        buff << "varying mediump vec2 Out_Tex0;\n";

    if (flags & FLAG_TEXMATRIX)
        buff << "uniform mat3 NormalMatrix;\n";

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
        buff << "uniform lowp float EnvMapCoefficient;\n";

    if (flags & FLAG_ENVMAP)
        buff << "varying mediump vec2 Out_Tex1;\n";

    if (flags & (FLAG_SPHERE_ENVMAP | FLAG_WATER))
        buff << "varying mediump vec3 Out_Refl;\n";

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_WATER | FLAG_SPHERE_XFORM))
        buff << "uniform vec3 CameraPosition;\n";

    if (flags & FLAG_FOG)
    {
        buff    << "varying mediump float Out_FogAmt;"
                << "uniform vec3 FogDistances;";

        if (screenFog) {
            buff << "varying mediump vec4 Out_ClipSpace;";
        }
    }

    if (flags & FLAG_WATER)
    {
        buff    << "uniform vec3 WaterSpecs;"
                << "varying mediump vec2 Out_WaterDetail;"
                << "varying mediump vec2 Out_WaterDetail2;"
                << "varying mediump float Out_WaterAlphaBlend;";
    }

    if (flags & FLAG_COLOR2)
    {
        buff    << "attribute vec4 Color2;"
                << "uniform lowp float ColorInterp;";
    }

    if ( flags & (FLAG_COLOR | FLAG_LIGHTING))
        buff << "varying lowp vec4 Out_Color;";

    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)))
        buff << "varying lowp vec3 Out_Spec;";

    buff << "void main() {";
    if (flags & (FLAG_BONE3 | FLAG_BONE4))
    {
        buff    << "\tivec4 BlendIndexArray = ivec4(BoneIndices);"
                << "\tmat4 BoneToLocal;"
                << "\tBoneToLocal[0] = Bones[BlendIndexArray.x*3] * BoneWeight.x;"
                << "\tBoneToLocal[1] = Bones[BlendIndexArray.x*3+1] * BoneWeight.x;"
                << "\tBoneToLocal[2] = Bones[BlendIndexArray.x*3+2] * BoneWeight.x;"
                << "\tBoneToLocal[3] = vec4(0.0,0.0,0.0,1.0);"
                << "\tBoneToLocal[0] += Bones[BlendIndexArray.y*3] * BoneWeight.y;"
                << "\tBoneToLocal[1] += Bones[BlendIndexArray.y*3+1] * BoneWeight.y;"
                << "\tBoneToLocal[2] += Bones[BlendIndexArray.y*3+2] * BoneWeight.y;"
                << "\tBoneToLocal[0] += Bones[BlendIndexArray.z*3] * BoneWeight.z;"
                << "\tBoneToLocal[1] += Bones[BlendIndexArray.z*3+1] * BoneWeight.z;"
                << "\tBoneToLocal[2] += Bones[BlendIndexArray.z*3+2] * BoneWeight.z;";

        if (flags & FLAG_BONE4)
        {
            buff    << "\tBoneToLocal[0] += Bones[BlendIndexArray.w*3] * BoneWeight.w;"
                    << "\tBoneToLocal[1] += Bones[BlendIndexArray.w*3+1] * BoneWeight.w;"
                    << "\tBoneToLocal[2] += Bones[BlendIndexArray.w*3+2] * BoneWeight.w;";
        }
        buff << "\tvec4 WorldPos = ObjMatrix * (vec4(Position,1.0) * BoneToLocal);";
    }
    else
    {
        buff << "\tvec4 WorldPos = ObjMatrix * vec4(Position,1.0);";
    }

    if (flags & FLAG_SPHERE_XFORM)
    {
        buff    << "    vec3 ReflVector = WorldPos.xyz - CameraPosition.xyz;"
                << "\tvec3 ReflPos = normalize(ReflVector);"
                << "    ReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);";

        if (screenFog) {
            buff    << "    float coefOffset = 1.0 - clamp(length(ReflVector.xy) / 45.0, 0.0, 1.0);"
                    << "    coefOffset = -1000.0 * coefOffset;"
                    << "\tgl_Position = vec4(ReflPos.xy, coefOffset + length(ReflVector) * 0.002, 1.0);";
        } else {
            buff << "\tgl_Position = vec4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);";
        }
    }
    else
    {
        buff << "\tvec4 ViewPos = ViewMatrix * WorldPos;";

        buff << "\tgl_Position = ProjMatrix * ViewPos;";
    }

    if ((flags & FLAG_FOG) && screenFog) {
        buff << "\tOut_ClipSpace = gl_Position;";
    }

    if (flags & FLAG_LIGHTING)
    {
        if (((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) && (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)))
            buff << "vec3 WorldNormal = normalize(vec3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;";
        else
        {
            if (flags & (FLAG_BONE3 | FLAG_BONE4))
                buff << "vec3 WorldNormal = mat3(ObjMatrix) * (Normal * mat3(BoneToLocal));";
            else
                buff << "vec3 WorldNormal = (ObjMatrix * vec4(Normal,0.0)).xyz;";
        }
    }
    else
    {
        if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_WATER))
        {
            buff << "vec3 WorldNormal = (ObjMatrix * vec4(Normal,0.0)).xyz;";
        }
        else
        {
            if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
                buff << "vec3 WorldNormal = vec3(0.0, 0.0, 0.0);";
        }
    }

    if (flags & FLAG_FOG) {
        const float fogBase = RQShader::g_fFogHeightBase;
        const float fogSpan = std::max(RQShader::g_fFogHeightTop - fogBase, 1.0f);

        buff << "float fogDist = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 1.0);"
             << "float fogHeight = clamp((" << GlslFloat(fogBase + fogSpan) << " - WorldPos.z) / " << GlslFloat(fogSpan) << ", 0.0, 1.0);"
             << "fogHeight = pow(fogHeight, " << GlslFloat(RQShader::g_fFogHeightPow) << ");"
             << "Out_FogAmt = fogDist * fogHeight;";
    }

    if (flags & FLAG_TEX0) {
        char* tex;

        if (flags & FLAG_PROJECT_TEXCOORD)
            tex = "TexCoord0.xy / TexCoord0.w";

        else if (flags & FLAG_COMPRESSED_TEXCOORD)
            tex = "TexCoord0 / 512.0";
        else
            tex = "TexCoord0";

        if (flags & FLAG_TEXMATRIX)
            buff << "Out_Tex0 = (NormalMatrix * vec3(" << tex << ", 1.0)).xy;";
        else
            buff << "Out_Tex0 = " << tex << ";";
    }

    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_WATER))
    {
        buff    << "vec3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);"
                << "reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;";

        if (flags & (FLAG_SPHERE_ENVMAP | FLAG_WATER))
            buff << "Out_Refl = reflVector;";
        else
            buff << "Out_Tex1 = vec2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);";
    }

    if (flags & FLAG_COLOR2)
    {
        buff << "lowp vec4 InterpColor = mix(GlobalColor, Color2, ColorInterp);";
        vertexColor = "InterpColor";
    }
    else
    {
        vertexColor = "GlobalColor";
    }

    if (flags & FLAG_LIGHTING) {

        buff    << "vec3 ambEmissLight = vec3(0.0, 0.0, 0.0);"
                << "vec3 diffColor = vec3(0.0, 0.0, 0.0);";

        if (flags & FLAG_COLOR_EMISSIVE) {
            if(flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)){
                buff << "ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + " << vertexColor << ".xyz * MaterialDiffuse.xyz;";
            }else
            if (flags & FLAG_CAMERA_BASED_NORMALS){
                buff    << "vec3 vertClamped = clamp(" << vertexColor <<".xyz, 0.0, 0.5);"
                        << "ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + vertClamped;";
            }else
                buff << "ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + " << vertexColor <<".xyz;";
        } else {
            buff << "ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + MaterialEmissive.xyz;";
        }

        if (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)) {
            if (flags & FLAG_LIGHT1) {
                if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
                    buff << "diffColor += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;";
                else
                    buff << "diffColor += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;";
            }
            if (flags & FLAG_LIGHT2)
                buff << "diffColor += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;";
            if (flags & FLAG_LIGHT3)
                buff << "diffColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;";

            buff << "diffColor *= MaterialDiffuse.xyz;";
        }

        if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
            buff    << "Out_Color = vec4(ambEmissLight + diffColor, MaterialAmbient.w * " << vertexColor <<".w);"
                    << "Out_Color = clamp(Out_Color, 0.0, 1.0);";
        }
    } else {
        if (flags & (FLAG_COLOR | FLAG_LIGHTING))
            buff << "Out_Color = " << vertexColor << ";";
    }

    if (!RQCaps->unk_08 && (flags & FLAG_LIGHT1)) {
        if (flags & ( FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
        {
            char* v9 = const_cast<char *>(RQCaps->isMaliChip ? "9.0" : "10.0");

            buff    << "float specAmt = max(pow(dot(reflVector, DirLightDirection), " << v9 << "), 0.0) * EnvMapCoefficient * "
                    << (screenFog ? "1.3" : "2.0") << ";"
                    << "Out_Spec = specAmt * DirLightDiffuseColor;";
        }
    }

    if (flags & FLAG_WATER)
    {
        buff    << "Out_WaterDetail = (Out_Tex0 * 4.0) + vec2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);"
                << "Out_WaterDetail2 = (Out_Tex0 * -8.0) + vec2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);";

        if (screenFog) {
            buff << "Out_WaterAlphaBlend = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;";
        } else {
            buff << "Out_WaterAlphaBlend = distance(WorldPos.xy, CameraPosition.xy);";
        }
    }

    buff << "}";

    strcpy(vertexSrc.shaderSrc, buff.str().c_str());
}