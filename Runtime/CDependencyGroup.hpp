#pragma once

#include <vector>
#include "Runtime/CFactoryMgr.hpp"

namespace urde {
class CDependencyGroup {
  std::vector<SObjectTag> x0_objectTags;

public:
  explicit CDependencyGroup(CInputStream& in);
  void ReadFromStream(CInputStream& in);
  const std::vector<SObjectTag>& GetObjectTagVector() const { return x0_objectTags; }
};

CFactoryFnReturn FDependencyGroupFactory(const SObjectTag& tag, CInputStream& in, const CVParamTransfer& param,
                                         CObjectReference* selfRef);
} // namespace urde
