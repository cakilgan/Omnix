#include "OmnixEvents.h"
#include "OmnixUtil.h"
#include "boltlog.h"
#include <Omnix.h>
#include <vector>

OmnixEventBus& Omnix::Core::Omnix::eventBus(){
    return EVENT_BUS;
}
BL::Default::Logger& Omnix::Core::Omnix::globalLogger(){
    return GLOBAL_LOGGER;
}
std::vector<std::shared_ptr<Omnix::Core::OmnixModule>>& Omnix::Core::Omnix::getModules(){
    return MODULES;
}

OmnixResult Omnix::Core::Omnix::installModules(){
    OmnixResult result{ResultParent::EVENT,"Omnix#installModules"};
    inScopeOf = &result;
    RESULT(InstallModuleResult, ResultParent::EVENT){
        for(auto mod:getModules()){
           InstallModuleResult.addChild(mod->install(*this));
        }  
    }
    std::vector<OmnixResultContext> cxs;
    if(result.hasError(cxs)){
        LOG_FATAL(globalLogger())<<"has error on results <?>\n";
        for(auto cx:cxs){
            globalLogger()<<": "<<cx.msg<<"\n";
        }
        globalLogger()<<blENDL;
    }
    inScopeOf = nullptr;
    return result;
}
OmnixResult Omnix::Core::Omnix::uninstallModules(){
    OmnixResult result{ResultParent::EVENT,"Omnix$uninstallModules"};
    inScopeOf = &result;
    RESULT(UninstallModuleResult, ResultParent::EVENT){
        for(auto mod:getModules()){
            UninstallModuleResult.addChild(mod->uninstall(*this));
        }
    }
    std::vector<OmnixResultContext> cxs;
    if(result.hasError(cxs)){
        LOG_FATAL(globalLogger())<<"has error on results <?>\n";
        for(auto cx:cxs){
            globalLogger()<<": "<<cx.msg<<"\n";
        }
        globalLogger()<<blENDL;
    }
    inScopeOf = nullptr;
    return result;
}