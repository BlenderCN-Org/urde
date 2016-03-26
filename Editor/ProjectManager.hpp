#ifndef URDE_PROJECT_MANAGER_HPP
#define URDE_PROJECT_MANAGER_HPP

#include <hecl/Database.hpp>
#include <athena/DNAYaml.hpp>
#include "ProjectResourceFactoryMP1.hpp"
#include "Runtime/CSimplePool.hpp"

namespace urde
{
class ViewManager;

using ConfigReader = athena::io::YAMLDocReader;
using ConfigWriter = athena::io::YAMLDocWriter;

class ProjectManager
{
    ViewManager& m_vm;
    std::unique_ptr<hecl::Database::Project> m_proj;
    static bool m_registeredSpecs;
    ProjectResourceFactoryMP1 m_factoryMP1;
    urde::CSimplePool m_objStore;

public:
    ProjectManager(ViewManager& vm);
    operator bool() const {return m_proj.operator bool();}

    hecl::Database::Project* project() {return m_proj.get();}

    bool newProject(const hecl::SystemString& path);
    bool openProject(const hecl::SystemString& path);
    bool extractGame(const hecl::SystemString& path);

    bool saveProject();

};

}

#endif // URDE_PROJECT_MANAGER_HPP
