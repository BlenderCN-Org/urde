#include "shader_CFluidPlaneShader.hpp"
#include <fmt/ostream.h>
#include <sstream>

#define FOG_STRUCT_GLSL                                                                                                \
  "struct Fog\n"                                                                                                       \
  "{\n"                                                                                                                \
  "    vec4 color;\n"                                                                                                  \
  "    float A;\n"                                                                                                     \
  "    float B;\n"                                                                                                     \
  "    float C;\n"                                                                                                     \
  "    int mode;\n"                                                                                                    \
  "    float indScale;\n"                                                                                              \
"};\n"

#define FOG_ALGORITHM_GLSL                                                                                             \
  "vec4 MainPostFunc(vec4 colorIn)\n"                                                                                  \
  "{\n"                                                                                                                \
  "    float fogZ;\n"                                                                                                  \
  "    float fogF = clamp((fog.A / (fog.B - gl_FragCoord.z)) - fog.C, 0.0, 1.0);\n"                                    \
  "    switch (fog.mode)\n"                                                                                            \
  "    {\n"                                                                                                            \
  "    case 2:\n"                                                                                                      \
  "        fogZ = fogF;\n"                                                                                             \
  "        break;\n"                                                                                                   \
  "    case 4:\n"                                                                                                      \
  "        fogZ = 1.0 - exp2(-8.0 * fogF);\n"                                                                          \
  "        break;\n"                                                                                                   \
  "    case 5:\n"                                                                                                      \
  "        fogZ = 1.0 - exp2(-8.0 * fogF * fogF);\n"                                                                   \
  "        break;\n"                                                                                                   \
  "    case 6:\n"                                                                                                      \
  "        fogZ = exp2(-8.0 * (1.0 - fogF));\n"                                                                        \
  "        break;\n"                                                                                                   \
  "    case 7:\n"                                                                                                      \
  "        fogF = 1.0 - fogF;\n"                                                                                       \
  "        fogZ = exp2(-8.0 * fogF * fogF);\n"                                                                         \
  "        break;\n"                                                                                                   \
  "    default:\n"                                                                                                     \
  "        fogZ = 0.0;\n"                                                                                              \
  "        break;\n"                                                                                                   \
  "    }\n"                                                                                                            \
  "#if IS_ADDITIVE\n"                                                                                                  \
  "    return vec4(mix(colorIn, vec4(0.0), clamp(fogZ, 0.0, 1.0)).rgb, colorIn.a);\n"                                  \
  "#else\n"                                                                                                            \
  "    return vec4(mix(colorIn, fog.color, clamp(fogZ, 0.0, 1.0)).rgb, colorIn.a);\n"                                  \
  "#endif\n"                                                                                                           \
  "}\n"

static const char* VS =
    "layout(location=0) in vec4 posIn;\n"
    "layout(location=1) in vec4 normalIn;\n"
    "layout(location=2) in vec4 binormalIn;\n"
    "layout(location=3) in vec4 tangentIn;\n"
    "layout(location=4) in vec4 colorIn;\n"
    "\n"
    "UBINDING0 uniform FluidPlaneUniform\n"
    "{\n"
    "    mat4 mv;\n"
    "    mat4 mvNorm;\n"
    "    mat4 proj;\n"
    "    mat4 texMtxs[6];\n"
    "};\n"
    "\n"
    "struct VertToFrag\n"
    "{\n"
    "    vec4 mvPos;\n"
    "    vec4 mvNorm;\n"
    "    vec4 mvBinorm;\n"
    "    vec4 mvTangent;\n"
    "    vec4 color;\n"
    "    vec2 uvs[7];\n"
    "};\n"
    "\n"
    "SBINDING(0) out VertToFrag vtf;\n"
    "void main()\n"
    "{\n"
    "    vec4 pos = vec4(posIn.xyz, 1.0);\n"
    "    vtf.mvPos = mv * pos;\n"
    "    gl_Position = proj * vtf.mvPos;\n"
    "    vtf.mvNorm = mvNorm * normalIn;\n"
    "    vtf.mvBinorm = mvNorm * binormalIn;\n"
    "    vtf.mvTangent = mvNorm * tangentIn;\n"
    "    vtf.color = vec4(colorIn.xyz, 1.0);\n"
    "    vtf.uvs[0] = (texMtxs[0] * pos).xy;\n"
    "    vtf.uvs[1] = (texMtxs[1] * pos).xy;\n"
    "    vtf.uvs[2] = (texMtxs[2] * pos).xy;\n"
    "    ADDITIONAL_TCGS\n" // Additional TCGs here
    "}\n";

static const char* TessVS =
    "layout(location=0) in vec4 posIn;\n"
    "layout(location=1) in vec4 outerLevelsIn;\n"
    "layout(location=2) in vec2 innerLevelsIn;\n"
    "\n"
    "struct VertToControl\n"
    "{\n"
    "    vec4 minMaxPos;\n"
    "    vec4 outerLevels;\n"
    "    vec2 innerLevels;\n"
    "};\n"
    "\n"
    "SBINDING(0) out VertToControl vtc;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vtc.minMaxPos = posIn;\n"
    "    vtc.outerLevels = outerLevelsIn;\n"
    "    vtc.innerLevels = innerLevelsIn;\n"
    "}\n";

static const char* TessCS =
    "#extension GL_ARB_tessellation_shader: enable\n"
    "layout(vertices = 1) out;\n"
    "\n"
    "struct VertToControl\n"
    "{\n"
    "    vec4 minMaxPos;\n"
    "    vec4 outerLevels;\n"
    "    vec2 innerLevels;\n"
    "};\n"
    "\n"
    "SBINDING(0) in VertToControl vtc[];\n"
    "SBINDING(0) patch out vec4 minMaxPos;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    minMaxPos = vtc[gl_InvocationID].minMaxPos;\n"
    "    for (int i=0 ; i<4 ; ++i)\n"
    "        gl_TessLevelOuter[i] = vtc[gl_InvocationID].outerLevels[i];\n"
    "    for (int i=0 ; i<2 ; ++i)\n"
    "        gl_TessLevelInner[i] = vtc[gl_InvocationID].innerLevels[i];\n"
    "}";

static const char* TessES =
    "#extension GL_ARB_tessellation_shader: enable\n"
    "layout(quads, equal_spacing) in;\n"
    "\n"
    "struct Ripple\n"
    "{\n"
    "    vec4 center; // time, distFalloff\n"
    "    vec4 params; // amplitude, lookupPhase, lookupTime\n"
    "};\n"
    "\n"
    "UBINDING0 uniform FluidPlaneUniform\n"
    "{\n"
    "    mat4 mv;\n"
    "    mat4 mvNorm;\n"
    "    mat4 proj;\n"
    "    mat4 texMtxs[6];\n"
    "    Ripple ripples[20];\n"
    "    vec4 colorMul;\n"
    "    float rippleNormResolution;\n"
    "};\n"
    "\n"
    "struct VertToFrag\n"
    "{\n"
    "    vec4 mvPos;\n"
    "    vec4 mvNorm;\n"
    "    vec4 mvBinorm;\n"
    "    vec4 mvTangent;\n"
    "    vec4 color;\n"
    "    vec2 uvs[7];\n"
    "};\n"
    "\n"
    "SBINDING(0) patch in vec4 minMaxPos;\n"
    "SBINDING(0) out VertToFrag vtf;\n"
    "\n"
    "RIPPLE_TEXTURE_BINDING uniform sampler2D RippleMap;\n"
    "\n"
    "const float PI_X2 = 6.283185307179586;\n"
    "\n"
    "void ApplyRipple(in Ripple ripple, in vec2 pos, inout float height)\n"
    "{\n"
    "    float dist = length(ripple.center.xy - pos);\n"
    "    float rippleV = textureLod(RippleMap, vec2(dist * ripple.center.w, ripple.center.z), 0.0).r;\n"
    "    height += rippleV * ripple.params.x * sin((dist * ripple.params.y + ripple.params.z) * PI_X2);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 posIn = vec2(mix(minMaxPos.x, minMaxPos.z, gl_TessCoord.x),\n"
    "                      mix(minMaxPos.y, minMaxPos.w, gl_TessCoord.y));\n"
    "    float height = 0.0;\n"
    "    float upHeight = 0.0;\n"
    "    float downHeight = 0.0;\n"
    "    float rightHeight = 0.0;\n"
    "    float leftHeight = 0.0;\n"
    "    for (int i=0 ; i<20 ; ++i)\n"
    "    {\n"
    "        ApplyRipple(ripples[i], posIn, height);\n"
    "        ApplyRipple(ripples[i], posIn + vec2(0.0, rippleNormResolution), upHeight);\n"
    "        ApplyRipple(ripples[i], posIn - vec2(0.0, rippleNormResolution), downHeight);\n"
    "        ApplyRipple(ripples[i], posIn + vec2(rippleNormResolution, 0.0), rightHeight);\n"
    "        ApplyRipple(ripples[i], posIn - vec2(rippleNormResolution, 0.0), leftHeight);\n"
    "    }\n"
    "    vec4 normalIn = vec4(normalize(vec3((leftHeight - rightHeight),\n"
    "                                        (downHeight - upHeight),\n"
    "                                        rippleNormResolution)), 1.0);\n"
    "    vec4 binormalIn = vec4(normalIn.x, normalIn.z, -normalIn.y, 1.0);\n"
    "    vec4 tangentIn = vec4(normalIn.z, normalIn.y, -normalIn.x, 1.0);\n"
    "    vec4 pos = vec4(posIn, height, 1.0);\n"
    "    vtf.mvPos = mv * pos;\n"
    "    gl_Position = proj * vtf.mvPos;\n"
    "    vtf.mvNorm = mvNorm * normalIn;\n"
    "    vtf.mvBinorm = mvNorm * binormalIn;\n"
    "    vtf.mvTangent = mvNorm * tangentIn;\n"
    "    vtf.color = max(height, 0.0) * colorMul;\n"
    "    vtf.color.a = 1.0;\n"
    "    vtf.uvs[0] = (texMtxs[0] * pos).xy;\n"
    "    vtf.uvs[1] = (texMtxs[1] * pos).xy;\n"
    "    vtf.uvs[2] = (texMtxs[2] * pos).xy;\n"
    "    ADDITIONAL_TCGS\n" // Additional TCGs here
    "}\n";

static const char* FS =
"struct Light\n"
"{\n"
"    vec4 pos;\n"
"    vec4 dir;\n"
"    vec4 color;\n"
"    vec4 linAtt;\n"
"    vec4 angAtt;\n"
"};\n"
FOG_STRUCT_GLSL
"\n"
"UBINDING2 uniform LightingUniform\n"
"{\n"
"    Light lights[" _XSTR(URDE_MAX_LIGHTS) "];\n"
"    vec4 ambient;\n"
"    vec4 kColor0;\n"
"    vec4 kColor1;\n"
"    vec4 kColor2;\n"
"    vec4 kColor3;\n"
"    vec4 addColor;\n"
"    Fog fog;\n"
"};\n"
"\n"
"vec4 LightingFunc(vec3 mvPosIn, vec3 mvNormIn)\n"
"{\n"
"    vec4 ret = ambient;\n"
"    \n"
"    for (int i=0 ; i<" _XSTR(URDE_MAX_LIGHTS) " ; ++i)\n"
"    {\n"
"        vec3 delta = mvPosIn - lights[i].pos.xyz;\n"
"        float dist = length(delta);\n"
"        vec3 deltaNorm = delta / dist;\n"
"        float angDot = max(dot(deltaNorm, lights[i].dir.xyz), 0.0);\n"
"        float att = 1.0 / (lights[i].linAtt[2] * dist * dist +\n"
"                           lights[i].linAtt[1] * dist +\n"
"                           lights[i].linAtt[0]);\n"
"        float angAtt = lights[i].angAtt[2] * angDot * angDot +\n"
"                       lights[i].angAtt[1] * angDot +\n"
"                       lights[i].angAtt[0];\n"
"        ret += lights[i].color * angAtt * att * max(dot(-deltaNorm, mvNormIn), 0.0);\n"
"    }\n"
"    \n"
"    return clamp(ret, 0.0, 1.0);\n"
"}\n"
"\n"
"struct VertToFrag\n"
"{\n"
"    vec4 mvPos;\n"
"    vec4 mvNorm;\n"
"    vec4 mvBinorm;\n"
"    vec4 mvTangent;\n"
"    vec4 color;\n"
"    vec2 uvs[7];\n"
"};\n"
"\n"
"SBINDING(0) in VertToFrag vtf;\n"
FOG_ALGORITHM_GLSL
"\n"
"layout(location=0) out vec4 colorOut;\n"
"TEXTURE_DECLS\n" // Textures here
"void main()\n"
"{\n"
"    vec4 lighting = LightingFunc(vtf.mvPos.xyz, normalize(vtf.mvNorm.xyz));\n"
"    COMBINER_EXPRS\n" // Combiner expression here
"    colorOut = MainPostFunc(colorOut);\n"
"}\n";

static const char* FSDoor =
"\n"
"struct Light\n"
"{\n"
"    vec4 pos;\n"
"    vec4 dir;\n"
"    vec4 color;\n"
"    vec4 linAtt;\n"
"    vec4 angAtt;\n"
"};\n"
FOG_STRUCT_GLSL
"\n"
"UBINDING2 uniform LightingUniform\n"
"{\n"
"    Light lights[" _XSTR(URDE_MAX_LIGHTS) "];\n"
"    vec4 ambient;\n"
"    vec4 kColor0;\n"
"    vec4 kColor1;\n"
"    vec4 kColor2;\n"
"    vec4 kColor3;\n"
"    vec4 addColor;\n"
"    Fog fog;\n"
"};\n"
"\n"
"struct VertToFrag\n"
"{\n"
"    vec4 mvPos;\n"
"    vec4 mvNorm;\n"
"    vec4 mvBinorm;\n"
"    vec4 mvTangent;\n"
"    vec4 color;\n"
"    vec2 uvs[7];\n"
"};\n"
FOG_ALGORITHM_GLSL
"\n"
"SBINDING(0) in VertToFrag vtf;\n"
"layout(location=0) out vec4 colorOut;\n"
"TEXTURE_DECLS\n" // Textures here
"void main()\n"
"{\n"
"    COMBINER_EXPRS\n" // Combiner expression here
"    colorOut = MainPostFunc(colorOut);\n"
"}\n";

static std::string _BuildFS(const SFluidPlaneShaderInfo& info) {
  std::stringstream out;
  int nextTex = 0;
  int nextTCG = 3;
  int bumpMapUv, envBumpMapUv, envMapUv, lightmapUv;

  out << "#define TEXTURE_DECLS ";
  if (info.m_hasPatternTex1)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D patternTex1;"), nextTex++);
  if (info.m_hasPatternTex2)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D patternTex2;"), nextTex++);
  if (info.m_hasColorTex)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D colorTex;"), nextTex++);
  if (info.m_hasBumpMap)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D bumpMap;"), nextTex++);
  if (info.m_hasEnvMap)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D envMap;"), nextTex++);
  if (info.m_hasEnvBumpMap)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D envBumpMap;"), nextTex++);
  if (info.m_hasLightmap)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D lightMap;"), nextTex++);
  out << '\n';

  if (info.m_hasBumpMap)
    bumpMapUv = nextTCG++;
  if (info.m_hasEnvBumpMap)
    envBumpMapUv = nextTCG++;
  if (info.m_hasEnvMap)
    envMapUv = nextTCG++;
  if (info.m_hasLightmap)
    lightmapUv = nextTCG;

  out << "#define COMBINER_EXPRS ";
  switch (info.m_type) {
  case EFluidType::NormalWater:
  case EFluidType::PhazonFluid:
  case EFluidType::Four:
    if (info.m_hasLightmap) {
      fmt::print(out, FMT_STRING("vec4 lightMapTexel = texture(lightMap, vtf.uvs[{}]);"), lightmapUv);
      // 0: Tex4TCG, Tex4, doubleLightmapBlend ? NULL : GX_COLOR1A1
      // ZERO, TEX, KONST, doubleLightmapBlend ? ZERO : RAS
      // Output reg 2
      // KColor 2
      if (info.m_doubleLightmapBlend) {
        // 1: Tex4TCG2, Tex4, GX_COLOR1A1
        // C2, TEX, KONST, RAS
        // Output reg 2
        // KColor 3
        // Tex * K2 + Lighting
        out << "lighting += mix(lightMapTexel * kColor2, lightMapTexel, kColor3);";
      } else {
        // mix(Tex * K2, Tex, K3) + Lighting
        out << "lighting += lightMapTexel * kColor2;";
      }
    }

    // Next: Tex0TCG, Tex0, GX_COLOR1A1
    // ZERO, TEX, KONST, RAS
    // Output reg prev
    // KColor 0

    // Next: Tex1TCG, Tex1, GX_COLOR0A0
    // ZERO, TEX, PREV, RAS
    // Output reg prev

    // Next: Tex2TCG, Tex2, GX_COLOR1A1
    // ZERO, TEX, hasTex4 ? C2 : RAS, PREV
    // Output reg prev

    // (Tex0 * kColor0 + Lighting) * Tex1 + VertColor + Tex2 * Lighting
    if (info.m_hasPatternTex2) {
      if (info.m_hasPatternTex1)
        out <<
            "colorOut = (texture(patternTex1, vtf.uvs[0]) * kColor0 + lighting) * "
            "texture(patternTex2, vtf.uvs[1]) + vtf.color;";
      else
        out << "colorOut = lighting * texture(patternTex2, vtf.uvs[1]) + vtf.color;";
    } else {
      out << "colorOut = vtf.color;";
    }

    if (info.m_hasColorTex && !info.m_hasEnvMap && info.m_hasEnvBumpMap) {
      // Make previous stage indirect, mtx0
      fmt::print(out, FMT_STRING(
          "vec2 indUvs = (texture(envBumpMap, vtf.uvs[{}]).ra - vec2(0.5, 0.5)) * "
          "vec2(fog.indScale, -fog.indScale);"),
          envBumpMapUv);
      out << "colorOut += texture(colorTex, indUvs + vtf.uvs[2]) * lighting;";
    } else if (info.m_hasEnvMap) {
      // Next: envTCG, envTex, NULL
      // PREV, TEX, KONST, ZERO
      // Output reg prev
      // KColor 1

      // Make previous stage indirect, mtx0
      if (info.m_hasColorTex)
        out << "colorOut += texture(colorTex, vtf.uvs[2]) * lighting;";
      fmt::print(out, FMT_STRING(
          "vec2 indUvs = (texture(envBumpMap, vtf.uvs[{}]).ra - vec2(0.5, 0.5)) * "
          "vec2(fog.indScale, -fog.indScale);"),
          envBumpMapUv);
      fmt::print(out, FMT_STRING("colorOut = mix(colorOut, texture(envMap, indUvs + vtf.uvs[{}]), kColor1);"), envMapUv);
    } else if (info.m_hasColorTex) {
      out << "colorOut += texture(colorTex, vtf.uvs[2]) * lighting;";
    }

    break;

  case EFluidType::PoisonWater:
    if (info.m_hasLightmap) {
      fmt::print(out, FMT_STRING("vec4 lightMapTexel = texture(lightMap, vtf.uvs[{}]);"), lightmapUv);
      // 0: Tex4TCG, Tex4, doubleLightmapBlend ? NULL : GX_COLOR1A1
      // ZERO, TEX, KONST, doubleLightmapBlend ? ZERO : RAS
      // Output reg 2
      // KColor 2
      if (info.m_doubleLightmapBlend) {
        // 1: Tex4TCG2, Tex4, GX_COLOR1A1
        // C2, TEX, KONST, RAS
        // Output reg 2
        // KColor 3
        // Tex * K2 + Lighting
        out << "lighting += mix(lightMapTexel * kColor2, lightMapTexel, kColor3);";
      } else {
        // mix(Tex * K2, Tex, K3) + Lighting
        out << "lighting += lightMapTexel * kColor2;";
      }
    }

    // Next: Tex0TCG, Tex0, GX_COLOR1A1
    // ZERO, TEX, KONST, RAS
    // Output reg prev
    // KColor 0

    // Next: Tex1TCG, Tex1, GX_COLOR0A0
    // ZERO, TEX, PREV, RAS
    // Output reg prev

    // Next: Tex2TCG, Tex2, GX_COLOR1A1
    // ZERO, TEX, hasTex4 ? C2 : RAS, PREV
    // Output reg prev

    // (Tex0 * kColor0 + Lighting) * Tex1 + VertColor + Tex2 * Lighting
    if (info.m_hasPatternTex2) {
      if (info.m_hasPatternTex1)
        out <<
            "colorOut = (texture(patternTex1, vtf.uvs[0]) * kColor0 + lighting) * "
            "texture(patternTex2, vtf.uvs[1]) + vtf.color;";
      else
        out << "colorOut = lighting * texture(patternTex2, vtf.uvs[1]) + vtf.color;";
    } else {
      out << "colorOut = vtf.color;";
    }

    if (info.m_hasColorTex) {
      if (info.m_hasEnvBumpMap) {
        // Make previous stage indirect, mtx0
        fmt::print(out, FMT_STRING(
            "vec2 indUvs = (texture(envBumpMap, vtf.uvs[{}]).ra - vec2(0.5, 0.5)) * "
            "vec2(fog.indScale, -fog.indScale);"),
            envBumpMapUv);
        out << "colorOut += texture(colorTex, indUvs + vtf.uvs[2]) * lighting;";
      } else {
        out << "colorOut += texture(colorTex, vtf.uvs[2]) * lighting;";
      }
    }

    break;

  case EFluidType::Lava:
    // 0: Tex0TCG, Tex0, GX_COLOR0A0
    // ZERO, TEX, KONST, RAS
    // Output reg prev
    // KColor 0

    // 1: Tex1TCG, Tex1, GX_COLOR0A0
    // ZERO, TEX, PREV, RAS
    // Output reg prev

    // 2: Tex2TCG, Tex2, NULL
    // ZERO, TEX, ONE, PREV
    // Output reg prev

    // (Tex0 * kColor0 + VertColor) * Tex1 + VertColor + Tex2
    if (info.m_hasPatternTex2) {
      if (info.m_hasPatternTex1)
        out <<
            "colorOut = (texture(patternTex1, vtf.uvs[0]) * kColor0 + vtf.color) * "
            "texture(patternTex2, vtf.uvs[1]) + vtf.color;";
      else
        out << "colorOut = vtf.color * texture(patternTex2, vtf.uvs[1]) + vtf.color;";
    } else {
      out << "colorOut = vtf.color;";
    }

    if (info.m_hasColorTex)
      out << "colorOut += texture(colorTex, vtf.uvs[2]);";

    if (info.m_hasBumpMap) {
      // 3: bumpMapTCG, bumpMap, NULL
      // ZERO, TEX, ONE, HALF
      // Output reg 0, no clamp, no bias

      // 4: bumpMapTCG2, bumpMap, NULL
      // ZERO, TEX, ONE, C0
      // Output reg 0, subtract, clamp, no bias

      out <<
          "vec3 lightVec = lights[3].pos.xyz - vtf.mvPos.xyz;"
          "float lx = dot(vtf.mvTangent.xyz, lightVec);"
          "float ly = dot(vtf.mvBinorm.xyz, lightVec);";
      fmt::print(out, FMT_STRING(
          "vec4 emboss1 = texture(bumpMap, vtf.uvs[{}]) + vec4(0.5);"
          "vec4 emboss2 = texture(bumpMap, vtf.uvs[{}] + vec2(lx, ly));"),
          bumpMapUv, bumpMapUv);

      // 5: NULL, NULL, NULL
      // ZERO, PREV, C0, ZERO
      // Output reg prev, scale 2, clamp

      // colorOut * clamp(emboss1 + 0.5 - emboss2, 0.0, 1.0) * 2.0
      out << "colorOut *= clamp((emboss1 + vec4(0.5) - emboss2) * vec4(2.0), vec4(0.0), vec4(1.0));";
    }

    break;

  case EFluidType::ThickLava:
    // 0: Tex0TCG, Tex0, GX_COLOR0A0
    // ZERO, TEX, KONST, RAS
    // Output reg prev
    // KColor 0

    // 1: Tex1TCG, Tex1, GX_COLOR0A0
    // ZERO, TEX, PREV, RAS
    // Output reg prev

    // 2: Tex2TCG, Tex2, NULL
    // ZERO, TEX, ONE, PREV
    // Output reg prev

    // (Tex0 * kColor0 + VertColor) * Tex1 + VertColor + Tex2
    if (info.m_hasPatternTex2) {
      if (info.m_hasPatternTex1)
        out <<
            "colorOut = (texture(patternTex1, vtf.uvs[0]) * kColor0 + vtf.color) * "
            "texture(patternTex2, vtf.uvs[1]) + vtf.color;";
      else
        out << "colorOut = vtf.color * texture(patternTex2, vtf.uvs[1]) + vtf.color;";
    } else {
      out << "colorOut = vtf.color;";
    }

    if (info.m_hasColorTex)
      out << "colorOut += texture(colorTex, vtf.uvs[2]);";

    if (info.m_hasBumpMap) {
      // 3: bumpMapTCG, bumpMap, NULL
      // ZERO, TEX, PREV, ZERO
      // Output reg prev, scale 2
      fmt::print(out, FMT_STRING("vec4 emboss1 = texture(bumpMap, vtf.uvs[{}]) + vec4(0.5);"), bumpMapUv);
      out << "colorOut *= emboss1 * vec4(2.0);";
    }

    break;
  }

  out << "colorOut.a = kColor0.a;\n";

  out << "#define IS_ADDITIVE " << int(info.m_additive) << '\n';
  out << FS;
  return out.str();
}

static void _BuildAdditionalTCGs(std::stringstream& out, const SFluidPlaneShaderInfo& info) {
  int nextTCG = 3;
  int nextMtx = 4;

  out << "#define ADDITIONAL_TCGS ";
  if (info.m_hasBumpMap)
    fmt::print(out, FMT_STRING("vtf.uvs[{}] = (texMtxs[0] * pos).xy;"), nextTCG++);
  if (info.m_hasEnvBumpMap)
    fmt::print(out, FMT_STRING("vtf.uvs[{}] = (texMtxs[3] * vec4(normalIn.xyz, 1.0)).xy;"), nextTCG++);
  if (info.m_hasEnvMap)
    fmt::print(out, FMT_STRING("vtf.uvs[{}] = (texMtxs[{}] * pos).xy;"), nextTCG++, nextMtx++);
  if (info.m_hasLightmap)
    fmt::print(out, FMT_STRING("vtf.uvs[{}] = (texMtxs[{}] * pos).xy;"), nextTCG, nextMtx);
  out << '\n';
}

static std::string _BuildVS(const SFluidPlaneShaderInfo& info, bool tessellation) {
  if (tessellation)
    return TessVS;

  std::stringstream out;
  _BuildAdditionalTCGs(out, info);
  out << VS;
  return out.str();
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildVS(in, tessellation);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildVS(in, tessellation);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::NX, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildVS(in, tessellation);
}

template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildFS(in);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildFS(in);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::NX, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return _BuildFS(in);
}

template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Control>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return TessCS;
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Control>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return TessCS;
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::NX, hecl::PipelineStage::Control>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return TessCS;
}

static std::string BuildES(const SFluidPlaneShaderInfo& info) {
  int nextTex = 0;
  if (info.m_hasPatternTex1)
    nextTex++;
  if (info.m_hasPatternTex2)
    nextTex++;
  if (info.m_hasColorTex)
    nextTex++;
  if (info.m_hasBumpMap)
    nextTex++;
  if (info.m_hasEnvMap)
    nextTex++;
  if (info.m_hasEnvBumpMap)
    nextTex++;
  if (info.m_hasLightmap)
    nextTex++;

  std::stringstream out;
  _BuildAdditionalTCGs(out, info);
  out << "#define RIPPLE_TEXTURE_BINDING TBINDING" << nextTex << '\n';
  out << TessES;
  return out.str();
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Evaluation>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return BuildES(in);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Evaluation>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return BuildES(in);
}
template <>
std::string StageObject_CFluidPlaneShader<hecl::PlatformType::NX, hecl::PipelineStage::Evaluation>::BuildShader(
    const SFluidPlaneShaderInfo& in, bool tessellation) {
  return BuildES(in);
}

static std::string _BuildVS(const SFluidPlaneDoorShaderInfo& info) {
  std::stringstream out;
  out << "#define ADDITIONAL_TCGS\n";
  out << VS;
  return out.str();
}

static std::string _BuildFS(const SFluidPlaneDoorShaderInfo& info) {
  int nextTex = 0;
  std::stringstream out;

  out << "#define TEXTURE_DECLS ";
  if (info.m_hasPatternTex1)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D patternTex1;"), nextTex++);
  if (info.m_hasPatternTex2)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D patternTex2;"), nextTex++);
  if (info.m_hasColorTex)
    fmt::print(out, FMT_STRING("TBINDING{} uniform sampler2D colorTex;"), nextTex++);
  out << '\n';

  // Tex0 * kColor0 * Tex1 + Tex2
  out << "#define COMBINER_EXPRS ";
  if (info.m_hasPatternTex1 && info.m_hasPatternTex2) {
    out <<
        "colorOut = texture(patternTex1, vtf.uvs[0]) * kColor0 * "
        "texture(patternTex2, vtf.uvs[1]);";
  } else {
    out << "colorOut = vec4(0.0);";
  }

  if (info.m_hasColorTex) {
    out << "colorOut += texture(colorTex, vtf.uvs[2]);";
  }

  out << "colorOut.a = kColor0.a;\n";

  out << "#define IS_ADDITIVE 0\n";
  out << FSDoor;
  return out.str();
}

template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildVS(in);
}
template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildVS(in);
}
template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::NX, hecl::PipelineStage::Vertex>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildVS(in);
}

template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::OpenGL, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildFS(in);
}
template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::Vulkan, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildFS(in);
}
template <>
std::string StageObject_CFluidPlaneDoorShader<hecl::PlatformType::NX, hecl::PipelineStage::Fragment>::BuildShader(
    const SFluidPlaneDoorShaderInfo& in) {
  return _BuildFS(in);
}
