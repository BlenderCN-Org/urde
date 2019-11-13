#include "Runtime/World/CScriptMazeNode.hpp"
#include "Runtime/CStateManager.hpp"
#include "Runtime/GameGlobalObjects.hpp"
#include "Runtime/Character/CModelData.hpp"
#include "Runtime/World/CActorParameters.hpp"
#include "TCastTo.hpp" // Generated file, do not modify include path

namespace urde {

atUint32 CScriptMazeNode::sMazeSeeds[300] = {0};

CScriptMazeNode::CScriptMazeNode(TUniqueId uid, std::string_view name, const CEntityInfo& info,
                                 const zeus::CTransform& xf, bool active, s32 w1, s32 w2, s32 w3,
                                 const zeus::CVector3f& vec1, const zeus::CVector3f& vec2, const zeus::CVector3f& vec3)
: CActor(uid, active, name, info, xf, CModelData::CModelDataNull(), CMaterialList(), CActorParameters::None(),
         kInvalidUniqueId)
, xe8_(w1)
, xec_(w1)
, xf0_(w2)
, x100_(vec1)
, x110_(vec2)
, x120_(vec3) {
  x13c_24_ = false;
  x13c_25_ = false;
  x13c_26_ = true;
}

void CScriptMazeNode::Accept(IVisitor& visitor) { visitor.Visit(this); }

void CScriptMazeNode::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId uid, CStateManager& mgr) {
  if (GetActive()) {
    switch (msg) {
    case EScriptObjectMessage::InitializedInArea:
      break;
    case EScriptObjectMessage::Deleted:
      break;
    case EScriptObjectMessage::Action:
      break;
    case EScriptObjectMessage::SetToZero:
      break;
    case EScriptObjectMessage::Deactivate:
      break;
    default:
      break;
    }
  }
  CActor::AcceptScriptMsg(msg, uid, mgr);
}

void CScriptMazeNode::Think(float dt, CStateManager& mgr) {
  if (!GetActive() || x13c_25_)
    return;

  xf8_ -= dt;
  if (xf8_ > 0.f)
    return;

  xf8_ = 0.f;
  if (x13c_26_) {
    x13c_26_ = false;
    __SendMsgToChildren(mgr, EScriptObjectMessage::Deactivate);
  } else {
    x13c_26_ = true;
    __SendMsgToChildren(mgr, EScriptObjectMessage::Activate);
  }
}

void CScriptMazeNode::__SendMsgToChildren(CStateManager& mgr, EScriptObjectMessage msg) {
  mgr.SendScriptMsg(mgr.ObjectById(x11c_), GetUniqueId(), msg);
  mgr.SendScriptMsg(mgr.ObjectById(xfc_), GetUniqueId(), msg);
  mgr.SendScriptMsg(mgr.ObjectById(x10c_), GetUniqueId(), msg);
  mgr.SendScriptMsg(mgr.ObjectById(xf4_), GetUniqueId(), msg);
}

void CScriptMazeNode::__FreeChildren(CStateManager& mgr) {
  mgr.FreeScriptObject(x11c_);
  mgr.FreeScriptObject(xfc_);
  mgr.FreeScriptObject(x10c_);
  mgr.FreeScriptObject(xf4_);
}

void CScriptMazeNode::LoadMazeSeeds() {
  const SObjectTag* tag = g_ResFactory->GetResourceIdByName("DUMB_MazeSeeds");
  u32 resSize = g_ResFactory->ResourceSize(*tag);
  std::unique_ptr<u8[]> buf = g_ResFactory->LoadResourceSync(*tag);
  CMemoryInStream in(buf.get(), resSize);
  for (u32 i = 0; i < 300; ++i)
    sMazeSeeds[i] = in.readUint32Big();
}
} // namespace urde
