#ifndef OMNIX_UTIL_H
#define OMNIX_UTIL_H
#include <corecrt.h>
#include <iostream>
#include <string>
#include <vector>
enum class ResultParent{
    MODULE,
    EVENT,
    TEST,
    INVALID,
    CUSTOM
};
enum class OmnixResultContextLevel{
    INFO,
    DEBUG,
    __ERROR,
    TRACE,
    LIFECYCLE
};

inline static std::string ResultParentToString(const ResultParent& parent){
    switch (parent) {
      case ResultParent::MODULE : return "MODULE";
      case ResultParent::EVENT: return "EVENT";
      case ResultParent::INVALID: return "INVALID";
      case ResultParent::TEST: return "TEST";
      case ResultParent::CUSTOM: return "CUSTOM";
    }
}
inline static std::string OmnixResultContextLevelToString(const OmnixResultContextLevel& contextLevel){
    switch (contextLevel) {
      case OmnixResultContextLevel::INFO: return "INFO";
      case OmnixResultContextLevel::DEBUG: return "DEBUG";
      case OmnixResultContextLevel::__ERROR: return "ERROR";
      case OmnixResultContextLevel::LIFECYCLE: return "LIFECYCLE";
      case OmnixResultContextLevel::TRACE: return "TRACE";
    }
} 
struct OmnixResultContext{
    OmnixResultContextLevel level;
    std::string msg;
};
struct OmnixResult{
    std::vector<OmnixResultContext> contexts{};
    std::vector<OmnixResult> childResults{};
    ResultParent parent;
    std::string resultname;
    public:
    OmnixResult(ResultParent parent):parent(parent){}
    OmnixResult(ResultParent parent,const std::string& name):parent(parent),resultname(name){}

    inline void addContext(const OmnixResultContext& cx){
        if(cx.level==OmnixResultContextLevel::TRACE){
            std::string trace = __FILE__;
            trace+=":";
            trace+=std::to_string(__LINE__);
            trace+=";; ";
            trace+=__FUNCTION__;

            OmnixResultContext refcx = cx;
            refcx.msg.append(" "+trace);
            contexts.push_back(refcx);
            return;
        }
        contexts.push_back(cx);
    }
    inline std::vector<OmnixResultContext> getContexts() const{
        return contexts;
    }
    inline void addChild(const OmnixResult& child){
        childResults.push_back(child);
    }
    inline const std::vector<OmnixResult>& getChildResults() const {
        return childResults;
    }
    inline ResultParent getParent() const {
        return parent;
    } 
    inline bool hasError(std::vector<OmnixResultContext>& cxs) const {
        for (const auto& cx : contexts)
            if (cx.level == OmnixResultContextLevel::__ERROR)
                cxs.push_back(cx);
        for (const auto& child : childResults)
            if (child.hasError(cxs))
              continue;
        return cxs.size()!=0;
    }
    inline std::string toString(int indent = 0) const {
        std::string tab(indent, ' ');
        std::string output = tab + "OmnixResult (" +resultname+"$"+ ResultParentToString(parent) + ")\n";
        for (const auto& cx : contexts) {
            output += tab + "  [" + OmnixResultContextLevelToString(cx.level) + "] " + cx.msg + "\n";
        }
        for (const auto& child : childResults) {
            output += child.toString(indent + 2);
        }
        return output;
     }
     inline OmnixResult* operator<<(const OmnixResultContext& cx){
        addContext(cx);
        return this;
     }
};

inline thread_local OmnixResult* inScopeOf;
struct InScopeOf{
    OmnixResult* parent;
    std::string selfstring;
    InScopeOf(OmnixResult* pscope,OmnixResult* parent){
        inScopeOf = pscope;
        selfstring = inScopeOf->toString();
        this->parent = parent;
    }
    ~InScopeOf(){
        if(!inScopeOf){
            std::cout<<"nullptr on parent "<<parent->toString()<<"\n";
            std::cout<<"debug:: "<<selfstring<<"\n";
            inScopeOf = parent;
            return;
        }
        parent->addChild(*inScopeOf);
        inScopeOf = parent;
    }
};
inline thread_local std::vector<OmnixResult*> __omnixResultStack;
struct ResultStack{
    ResultStack(OmnixResult* scopeof,OmnixResult* caseParent){
        if(!scopeof){
            std::cout<<"scopenull:: "<<caseParent->toString()<<std::endl;
        __omnixResultStack.push_back(caseParent);
        }else{
            __omnixResultStack.push_back(scopeof);
        }
    }
    ~ResultStack(){
        __omnixResultStack.pop_back();
    }
};


#define RESULT(NAME,MODULE_TYPE)\
 if (bool __omnix_once_##NAME = true)\
  for (OmnixResult NAME{MODULE_TYPE,#NAME}; __omnix_once_##NAME; __omnix_once_##NAME = false)\
    for (ResultStack _{inScopeOf,&NAME}; __omnix_once_##NAME; __omnix_once_##NAME = false)\
      for (InScopeOf _{&NAME,inScopeOf}; __omnix_once_##NAME; __omnix_once_##NAME = false)\



#define INFO_RESULT(B) OmnixResultContext{OmnixResultContextLevel::INFO,B}
#define DEBUG_RESULT(B) OmnixResultContext{OmnixResultContextLevel::DEBUG,B}
#define ERROR_RESULT(B) OmnixResultContext{OmnixResultContextLevel::ERROR,B}
#define LIFECYCLE_RESULT(B) OmnixResultContext{OmnixResultContextLevel::LIFECYCLE,B}

#endif // OMNIX_UTIL_H