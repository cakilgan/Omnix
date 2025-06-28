#ifndef OMNIX_DATA_H
#define OMNIX_DATA_H

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

class IDataProvider {
    public:
        virtual ~IDataProvider() = default;
    
        virtual const void* getData(const std::string& key) const = 0;

        using __variants = std::variant<int, std::string, float,bool>;
    
        virtual const void* getData(const std::vector<__variants>& keys) const = 0;

        virtual const __variants np_getData(const std::vector<__variants>& keys) const = 0;

};
class DataRegistry {
public:
    inline void registerProvider(const std::string& moduleName, std::shared_ptr<IDataProvider> provider) {
        providers[moduleName] = std::move(provider);
    }

    inline const void* requestData(const std::string& moduleName, const std::string& key) const {
        auto it = providers.find(moduleName);
        if (it != providers.end()) {
            return it->second->getData(key);
        }
        return nullptr;
    }


    template<typename variantTypes>
    inline const void* requestData(const std::string& moduleName,const std::vector<variantTypes>& keys){
        auto it = providers.find(moduleName);
        if(it !=providers.end()){
            return it->second->getData(keys);
        }
        return nullptr;
    }

    template<typename variantTypes>
    inline const variantTypes np_requestData(const std::string& moduleName,const std::vector<variantTypes>& keys){
        auto it = providers.find(moduleName);
        if(it !=providers.end()){
            return it->second->np_getData(keys);
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<IDataProvider>> providers;
};

#endif // OMNIX_DATA_H