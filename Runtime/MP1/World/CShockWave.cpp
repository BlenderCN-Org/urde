#include "Runtime/MP1/World/CShockWave.hpp"

#include "Runtime/Collision/CCollisionActor.hpp"
#include "Runtime/CSimplePool.hpp"
#include "Runtime/CStateManager.hpp"
#include "Runtime/GameGlobalObjects.hpp"
#include "Runtime/Graphics/CBooRenderer.hpp"
#include "Runtime/World/CActorParameters.hpp"
#include "Runtime/World/CGameLight.hpp"
#include "Runtime/World/CHUDBillboardEffect.hpp"
#include "Runtime/World/CPlayer.hpp"

#include "TCastTo.hpp" // Generated file, do not modify include path

namespace urde::MP1 {
CShockWave::CShockWave(TUniqueId uid, std::string_view name, const CEntityInfo& info, const zeus::CTransform& xf,
                       TUniqueId parent, const SShockWaveData& data, float minActiveTime, float knockback)
: CActor(uid, true, name, info, xf, CModelData::CModelDataNull(), {EMaterialTypes::Projectile},
         CActorParameters::None(), kInvalidUniqueId)
, xe8_id1(parent)
, xec_damageInfo(data.GetDamageInfo())
, x108_elementGenDesc(g_SimplePool->GetObj({SBIG('PART'), data.GetParticleDescId()}))
, x110_elementGen(std::make_unique<CElementGen>(x108_elementGenDesc))
, x114_data(data)
, x150_(data.GetX24())
, x154_(data.GetX2C())
, x15c_minActiveTime(minActiveTime)
, x160_knockback(knockback) {
  if (data.GetWeaponDescId().IsValid()) {
    x974_electricDesc = g_SimplePool->GetObj({SBIG('ELSC'), data.GetWeaponDescId()});
  }
  x110_elementGen->SetParticleEmission(true);
  x110_elementGen->SetOrientation(GetTransform().getRotation());
  x110_elementGen->SetGlobalTranslation(GetTranslation());
  xe6_27_thermalVisorFlags = 2;
}

void CShockWave::Accept(IVisitor& visitor) { visitor.Visit(this); }

void CShockWave::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId uid, CStateManager& mgr) {
  if (msg == EScriptObjectMessage::Registered) {
    if (x110_elementGen->SystemHasLight()) {
      x980_id2 = mgr.AllocateUniqueId();
      mgr.AddObject(new CGameLight(x980_id2, GetAreaIdAlways(), GetActive(), "ShockWaveLight_" + x10_name,
                                   GetTransform(), GetUniqueId(), x110_elementGen->GetLight(),
                                   x114_data.GetParticleDescId().Value(), 1, 0.f));
    }
  } else if (msg == EScriptObjectMessage::Deleted) {
    mgr.FreeScriptObject(x980_id2);
    x980_id2 = kInvalidUniqueId;
  }
  CActor::AcceptScriptMsg(msg, uid, mgr);
  mgr.SendScriptMsgAlways(x980_id2, uid, msg);
}

void CShockWave::AddToRenderer(const zeus::CFrustum& frustum, CStateManager& mgr) {
  CActor::AddToRenderer(frustum, mgr);
  g_Renderer->AddParticleGen(*x110_elementGen);
}

std::optional<zeus::CAABox> CShockWave::GetTouchBounds() const {
  if (x150_ <= 0.f) {
    return std::nullopt;
  }
  return zeus::CAABox({-x150_, -x150_, 0.f}, {x150_, x150_, 1.f}).getTransformedAABox(GetTransform());
}

void CShockWave::Render(CStateManager& mgr) {
  CActor::Render(mgr);
  x110_elementGen->Render();
}

void CShockWave::Think(float dt, CStateManager& mgr) {
  if (GetActive()) {
    x110_elementGen->Update(dt);
    x158_activeTime += dt;
    x150_ += x154_ * dt;
    x154_ += dt * x114_data.GetX30();
    x110_elementGen->SetExternalVar(0, x150_);
    for (size_t i = 0; i < x110_elementGen->GetNumActiveChildParticles(); ++i) {
      auto& particle = static_cast<CElementGen&>(x110_elementGen->GetActiveChildParticle(i));
      if (particle.Get4CharId() == SBIG('PART')) {
        particle.SetExternalVar(0, x150_);
      }
    }
    if (x16c_hitPlayerInAir) {
      x164_timeSinceHitPlayerInAir += dt;
      x16c_hitPlayerInAir = false;
    }
    if (x16d_hitPlayer) {
      x168_timeSinceHitPlayer += dt;
      x16d_hitPlayer = false;
    }
  }
  if (x110_elementGen->IsSystemDeletable() && x15c_minActiveTime > 0.f && x158_activeTime >= x15c_minActiveTime) {
    mgr.FreeScriptObject(GetUniqueId());
  } else if (x980_id2 != kInvalidUniqueId) {
    if (TCastToPtr<CGameLight> light = mgr.ObjectById(x980_id2)) {
      if (light->GetActive()) {
        light->SetLight(x110_elementGen->GetLight());
      }
    }
  }
}

void CShockWave::Touch(CActor& actor, CStateManager& mgr) {
  if (x158_activeTime >= x15c_minActiveTime) {
    return;
  }

  bool isParent = xe8_id1 == actor.GetUniqueId();
  if (TCastToConstPtr<CCollisionActor> cactor = mgr.GetObjectById(actor.GetUniqueId())) {
    isParent = xe8_id1 == cactor->GetOwnerId();
  }
  if (isParent) {
    return;
  }

  float mmax = x150_ * x150_;
  float mmin = mmax * x114_data.GetX28() * x114_data.GetX28();
  zeus::CVector3f dist = actor.GetTranslation() - GetTranslation();
  CDamageInfo damageInfo = xec_damageInfo;
  float knockBackScale = std::max(0.f, 1.f - x160_knockback * x158_activeTime);
  bool isPlayer = mgr.GetPlayer().GetUniqueId() == actor.GetUniqueId();
  bool isPlayerInAir = isPlayer && mgr.GetPlayer().GetPlayerMovementState() != CPlayer::EPlayerMovementState::OnGround;
  float distXYMag = dist.toVec2f().magSquared();
  if (distXYMag < mmin || distXYMag > mmax) {
    return;
  }

  if (isPlayer) {
    if (mgr.GetPlayer().GetPlayerMovementState() == CPlayer::EPlayerMovementState::OnGround) {
      const zeus::CTransform& playerTransform = mgr.GetPlayer().GetTransform();
      zeus::CVector3f playerDir = GetTranslation() - playerTransform.origin;
      if (playerDir.canBeNormalized()) {
        playerDir.normalize();
        float dot = std::abs(playerDir.dot(playerTransform.frontVector()));
        knockBackScale = std::max(0.12f, 0.88f * dot * dot);
      }
    }
    if (mgr.GetPlayer().GetVelocity().magnitude() > 40.f) {
      x168_timeSinceHitPlayer = 0.2666f;
    }
  }
  damageInfo.SetKnockBackPower(knockBackScale * damageInfo.GetKnockBackPower());

  if (isPlayer && (x164_timeSinceHitPlayerInAir >= 0.1333f || x168_timeSinceHitPlayer >= 0.2666f)) {
    return;
  }
  if (!IsHit(actor.GetUniqueId())) {
    mgr.ApplyDamage(GetUniqueId(), actor.GetUniqueId(), GetUniqueId(), damageInfo,
                    CMaterialFilter::MakeInclude({EMaterialTypes::Solid}), zeus::skZero3f);
    if (isPlayer && x974_electricDesc) {
      mgr.AddObject(new CHUDBillboardEffect(std::nullopt, x974_electricDesc, mgr.AllocateUniqueId(), true,
                                            "VisorElectricFx", CHUDBillboardEffect::GetNearClipDistance(mgr),
                                            CHUDBillboardEffect::GetScaleForPOV(mgr), zeus::skWhite, zeus::skOne3f,
                                            zeus::skZero3f));
      CSfxManager::SfxStart(x114_data.GetElectrocuteSfx(), 1.f, 1.f, false, 0x7f, false, kInvalidAreaId);
    }
    x170_hitIds.push_back(actor.GetUniqueId());
  } else {
    damageInfo.SetDamage(0.f);
    mgr.ApplyDamage(GetUniqueId(), actor.GetUniqueId(), GetUniqueId(), damageInfo,
                    CMaterialFilter::MakeInclude({EMaterialTypes::Solid}), zeus::skZero3f);
  }
  if (isPlayerInAir) {
    x16c_hitPlayerInAir = true;
  }
  if (isPlayer) {
    x16d_hitPlayer = true;
  }
}

bool CShockWave::IsHit(TUniqueId id) const {
  return std::find(x170_hitIds.begin(), x170_hitIds.end(), id) != x170_hitIds.end();
}
} // namespace urde::MP1
