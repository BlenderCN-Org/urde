#include "CBurstFire.hpp"

namespace urde
{
CBurstFire::CBurstFire(SBurst** bursts, s32 firstIndex)
    : x10_(firstIndex)
{
    SBurst** burst = bursts;
    while (1)
    {
        if (!*burst)
            break;

        x18_bursts.push_back(*burst);
        ++burst;
    }
}

void CBurstFire::Update(CStateManager&, float)
{

}

zeus::CVector3f CBurstFire::GetDistanceCompensatedError(float, float) const
{
    return {};
}
}