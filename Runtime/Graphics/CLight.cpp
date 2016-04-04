#include "CLight.hpp"
#include <cfloat>

namespace urde
{

static const zeus::CVector3f kDefaultPosition(0.f, 0.f, 0.f);
static const zeus::CVector3f kDefaultDirection(0.f, -1.f, 0.f);

float CLight::CalculateLightRadius() const
{
    if (x28_distL < FLT_EPSILON && x2c_distQ < FLT_EPSILON)
        return FLT_MAX;

    float intens = GetIntensity();

    if (x2c_distQ > FLT_EPSILON)
    {
        if (intens <= FLT_EPSILON)
            return 0.f;
        return std::sqrt(intens / 5.f * intens / 255.f * x2c_distQ);
    }

    float nextIntens = 5.f * intens /  255.f;
    return intens / std::min(0.2f, nextIntens) * x28_distL;
}

float CLight::GetIntensity() const
{
    if (x4c_24_intensityDirty)
    {
        ((CLight*)this)->x4c_24_intensityDirty = false;
        float coef = 1.f;
        if (x1c_type == ELightType::Custom)
            coef = x30_angleC;
        ((CLight*)this)->x48_cachedIntensity =
            coef * std::max(x18_color.r, std::max(x18_color.g, x18_color.b));
    }
    return x48_cachedIntensity;
}


CLight::CLight(const zeus::CVector3f& pos,
               const zeus::CVector3f& dir,
               const zeus::CColor& color,
               float distC, float distL, float distQ,
               float angleC, float angleL, float angleQ)
: x0_pos(pos), xc_dir(dir), x18_color(color),
  x1c_type(ELightType::Custom), x20_spotCutoff(0.f),
  x24_distC(distC), x28_distL(distL), x2c_distQ(distQ),
  x30_angleC(angleC), x34_angleL(angleL), x38_angleQ(angleQ),
  x44_cachedRadius(0.f), x48_cachedIntensity(0.f),
  x4c_24_intensityDirty(true), x4c_25_radiusDirty(true)
{}

CLight::CLight(ELightType type,
               const zeus::CVector3f& pos,
               const zeus::CVector3f& dir,
               const zeus::CColor& color,
               float cutoff)
: x0_pos(pos), xc_dir(dir), x18_color(color),
  x1c_type(type), x20_spotCutoff(cutoff),
  x24_distC(0.f), x28_distL(1.f), x2c_distQ(0.f),
  x30_angleC(0.f), x34_angleL(1.f), x38_angleQ(0.f),
  x44_cachedRadius(0.f), x48_cachedIntensity(0.f),
  x4c_24_intensityDirty(true), x4c_25_radiusDirty(true)
{
    switch (type)
    {
    case ELightType::Spot:
    {
        float cosCutoff = std::cos(cutoff * M_PI / 180.0);
        x30_angleC = 0.f;
        x34_angleL = -cosCutoff / (1.0 - cosCutoff);
        x38_angleQ = 1.f / (1.0 - cosCutoff);
        break;
    }
    case ELightType::Directional:
    {
        x24_distC = 1.f;
        x28_distL = 0.f;
        x2c_distQ = 0.f;
        break;
    }
    default: break;
    }
}

CLight CLight::BuildDirectional(const zeus::CVector3f& dir, const zeus::CColor& color)
{
    return CLight(ELightType::Directional, kDefaultPosition, dir, color, 180.f);
}

CLight CLight::BuildSpot(const zeus::CVector3f& pos, const zeus::CVector3f& dir,
                         const zeus::CColor& color, float angle)
{
    return CLight(ELightType::Spot, pos, dir, color, angle);
}

CLight CLight::BuildPoint(const zeus::CVector3f& pos, const zeus::CColor& color)
{
    return CLight(ELightType::Point, pos, kDefaultDirection, color, 180.f);
}

CLight CLight::BuildCustom(const zeus::CVector3f& pos, const zeus::CVector3f& dir,
                           const zeus::CColor& color,
                           float distC, float distL, float distQ,
                           float angleC, float angleL, float angleQ)
{
    return CLight(pos, dir, color, distC, distL, distQ, angleC, angleL, angleQ);
}

CLight CLight::BuildLocalAmbient(const zeus::CVector3f& pos, const zeus::CColor& color)
{
    return CLight(ELightType::LocalAmbient, pos, kDefaultDirection, color, 180.f);
}

}
