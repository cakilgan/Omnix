#ifndef OMNIX_MODULE_ID_H
#define OMNIX_MODULE_ID_H

#include "BoltID.h"
#include <vector>
enum class OmnixDependencyType{
    MODULE,
    EVENT,
    VARIABLE,
};
struct OmnixDependency{
    OmnixDependencyType type;
    std::string id;
    std::vector<OmnixDependency> dependencies;
};
struct OmnixModuleID{
    private:
    BoltID backend;
    std::string name="";
    std::string description="";
    std::vector<OmnixDependency> dependencies;


    OmnixModuleID():backend(BoltID::randomBoltID()){}

    OmnixModuleID(std::string name,std::string description):
    backend(BoltID::randomBoltID()),
    name(name),
    description(description){}

    OmnixModuleID(std::string name,std::string description,std::vector<OmnixDependency> dependencies):
    backend(BoltID::randomBoltID()),
    name(name),
    description(description),
    dependencies(dependencies){}

    public:
    inline static OmnixModuleID newModuleID(){
        return OmnixModuleID();
    }
    inline static OmnixModuleID newModuleID(std::string name,std::string description){
        return OmnixModuleID(name,description);
    }
    inline static OmnixModuleID newModuleID(std::string name,std::string description,std::vector<OmnixDependency> dependencies){
        return OmnixModuleID(name,description,dependencies);
    }
    inline std::string mod_name()const{
        return name;
    }
    inline std::string str() const{
        std::string rtr;
        rtr+= backend.toString();
        rtr+="::";
        rtr+=name;
        return rtr;
    }
    inline std::string desc() const{
        return description;
    }
    bool operator==(OmnixModuleID& other){
        return (this->backend==other.backend); 
    }
};

#endif // OMNIX_MODULE_ID_H