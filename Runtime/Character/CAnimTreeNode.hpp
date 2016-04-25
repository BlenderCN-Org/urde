#ifndef __URDE_CANIMTREENODE_HPP__
#define __URDE_CANIMTREENODE_HPP__

#include "IAnimReader.hpp"

namespace urde
{

class CAnimTreeNode : public IAnimReader
{
    std::string x4_name;
public:
    CAnimTreeNode(const std::string& name) : x4_name(name) {}
    bool IsCAnimTreeNode() const {return true;}

    virtual void Depth() const=0;
    virtual void VGetContributionOfHighestInfluence() const=0;
    virtual u32 VGetNumChildren() const=0;
    virtual std::shared_ptr<IAnimReader> VGetBestUnblendedChild() const=0;

    void GetContributionOfHighestInfluence() const;
    u32 GetNumChildren() const;
    std::shared_ptr<IAnimReader> GetBestUnblendedChild() const;
};

}

#endif // __URDE_CANIMTREENODE_HPP__