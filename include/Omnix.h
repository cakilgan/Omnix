#ifndef OMNIX_H
#define OMNIX_H

#include "OmnixEvents.h"
#include "OmnixModuleID.h"
#include "OmnixUtil.h"
#include "boltlog.h"
#include <memory>
#include <variant>


namespace Omnix{
    namespace Logging{
        const static std::shared_ptr<BL::FileSink> GLOBAL_LOG_FILE_SINK = {std::make_shared<BL::FileSink>("omnix_main")};
        const static std::shared_ptr<BL::ConsoleSink> GLOBAL_LOG_CONSOLE_SINK {std::make_shared<BL::ConsoleSink>()};
    };
    namespace Core{
        class OmnixModule;
        class Omnix;

        enum class OmnixState;
    };
    namespace Defaults {
        class OmnixControllerModule;
        class OmnixWindowModule;
    };
    namespace Backends{
        #ifdef  _WIN32
        class WindowsWindowBackend;
        #elif __linux__
        #elif __APPLE__
        #endif  //_WIN32
    };
    namespace Data{
    class IDataProvider;
    class DataRegistry;
    };
    namespace Helpers{
        template<typename  datatype>
        static const datatype* get_data(const std::string& from,const std::string& key);

        template<typename  datatype,typename varianttypes>
        static const datatype* get_data(const std::string& from,const std::vector<varianttypes>& keys);


        template<typename  datatype,typename varianttypes>
        static const datatype np_get_data(const std::string& from,const std::vector<varianttypes>& keys,const BoltID* id = nullptr);

    };

};
#include "OmnixData.h"

 namespace Omnix::Core{ 
        class Omnix{
        OmnixEventBus EVENT_BUS{};
        BL::Default::Logger GLOBAL_LOGGER = BL::Default::Logger("OmnixEngine");
        std::vector<std::shared_ptr<OmnixModule>> MODULES;
        public:
        Omnix(){
            GLOBAL_LOGGER.string_sinks.push_back(Logging::GLOBAL_LOG_CONSOLE_SINK);
            GLOBAL_LOGGER.string_sinks.push_back(Logging::GLOBAL_LOG_FILE_SINK);
        }
        inline static DataRegistry& data_instance() {
            static DataRegistry inst;
            return inst;
        }
        OmnixEventBus& eventBus();
        BL::Default::Logger& globalLogger();
        std::vector<std::shared_ptr<OmnixModule>>& getModules();
    
        OmnixResult installModules();
        OmnixResult uninstallModules();
        };

        class OmnixModule:public IDataProvider{
        OmnixModuleID moduleID;
        BL::Default::Logger moduleLogger;
        public:
        inline BL::Default::Logger& logger(){return moduleLogger;}
        inline OmnixModuleID& id(){return moduleID;}
        explicit OmnixModule(OmnixModuleID id):moduleID(id),moduleLogger(BL::Default::Logger(id.mod_name())){
            logger().string_sinks.push_back(Logging::GLOBAL_LOG_CONSOLE_SINK);
            logger().string_sinks.push_back(Logging::GLOBAL_LOG_FILE_SINK);
        }
        virtual OmnixResult install(Omnix& omnix) = 0;
        virtual OmnixResult uninstall(Omnix& omnix) = 0;
        virtual ~OmnixModule() = default;
        };
};
struct ScopedInScope {
    OmnixResult* oldScope;

    ScopedInScope(OmnixResult* newScope) {
        oldScope = inScopeOf;
        inScopeOf = newScope;
    }

    ~ScopedInScope() {
        inScopeOf = oldScope;
    }
};

template<typename  datatype>
static const datatype* Omnix::Helpers::get_data(const std::string& from,const std::string& key){
    auto data = static_cast<const datatype*>(Omnix::Core::Omnix::data_instance().requestData(from, key));
    return data;
}



template<typename  datatype,typename varianttypes>
static const datatype* Omnix::Helpers::get_data(const std::string& from,const std::vector<varianttypes>& keys){
    auto data = static_cast<const datatype*>(Omnix::Core::Omnix::data_instance().requestData(from, keys));
    return data;
}

template<typename  datatype,typename varianttypes>
static const datatype Omnix::Helpers::np_get_data(const std::string& from,const std::vector<varianttypes>& keys,const BoltID* id){
    auto a = Omnix::Core::Omnix::data_instance().np_requestData(from, keys,id);
    if(std::holds_alternative<datatype>(a)){
       auto result =  std::get<datatype>(a);
       return result;
    }
    return datatype();
}


#define DEFINE_INSTALL(MODULE_NAME) OmnixResult MODULE_NAME::install(Omnix::Omnix& omnix)override;




#define INSTALL(MODULE_NAME) \
OmnixResult MODULE_NAME::install(Omnix::Core::Omnix& omnix){ \
    OmnixResult result{ResultParent::MODULE, #MODULE_NAME "#install"};\
    ScopedInScope _(&result);\

#define END_INSTALL \
    return result; \
}

#define UNINSTALL(MODULE_NAME) \
OmnixResult MODULE_NAME::uninstall(Omnix::Core::Omnix& omnix){ \
    OmnixResult result{ResultParent::MODULE, #MODULE_NAME "#uninstall"};\
    ScopedInScope _(&result);\

#define END_UNINSTALL \
    return result; \
}

#define NP_DATA(MODULE_NAME,par ...)\
const IDataProvider::__variants MODULE_NAME::np_getData(par) const {\

    
#define END_NP_DATA \
return {};\
}\
    
#define DATA(MODULE_NAME,par ...) \
const void* MODULE_NAME::getData(par) const{\

#define END_DATA \
return nullptr;\
}\

#define RESULT_FUNC(FUNC_NAME,CLASS_NAME,ARGS ...)\
OmnixResult CLASS_NAME::FUNC_NAME(ARGS){ \
    OmnixResult result{ResultParent::MODULE, #CLASS_NAME};\
    ScopedInScope _(&result);\

#define  END_RESULT_FUNC  \
return result;\
}

#define OMNIX_EVENT_AUTO(EVENT_STRUCT, captures ...) { \
std::function<void(EVENT_STRUCT*)> __event = [this,&result, captures](EVENT_STRUCT* event){\
 std::string __resname__ = #EVENT_STRUCT;   \
 auto eventResult = OmnixResult(ResultParent::EVENT, __resname__);\
 
 // __resname__+= result.resultname;

#define OMNIX_EVENT_END_AUTO(parent) \
    eventResult<<OmnixResultContext{OmnixResultContextLevel::INFO,"from " parent};\
    result.addChild(eventResult); \
};\
omnix.eventBus().subscribe(__event); \
} \


#define OMNIX_EVENT(EVENT_STRUCT,EVENT_NAME,captures ...)\
std::function<void(EVENT_STRUCT*)> EVENT_NAME = [this,captures] (EVENT_STRUCT* event)\



#endif // OMNIX_H