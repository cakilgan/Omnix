#ifndef BRAIN_H
#define BRAIN_H


#include "BoltID.h"
#include "boltlog.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <any>
#include <fstream>
#include <iostream>
#include <token_utils.h>
#include <vector>
#include <filesystem>

namespace brain{
    namespace core
    {
    
        struct ResourceMetadataBase {
            std::string dataname;
            virtual void format() = 0;
            virtual ~ResourceMetadataBase() = default;
        };
        
        template<typename T>
        struct ResourceMetadata : ResourceMetadataBase {
            T data{};
            void format() override {}
        };

        struct BoolResourceMetadata:ResourceMetadata<std::atomic_bool>{
            BoolResourceMetadata(const std::string& dataname,const bool& starter){
                data.store(starter);
                this->dataname = dataname;
            }
            void format() override{};
        };
        struct StringResourceMetadata:ResourceMetadata<std::string>{
            StringResourceMetadata(const std::string& dataname,const std::string& starter){
                data = starter;
                this->dataname = dataname;
            }
            void format() override{};
        };
        struct IntResourceMetadata:ResourceMetadata<std::atomic_int>{
            IntResourceMetadata(const std::string& dataname,const int& starter){
                data.store(starter);
                this->dataname = dataname;
            }
            void format() override{};
        };
        
        
    
        struct ResourceID{
            std::string basic_id;
            BoltID backend;
        };
    
        struct ResourceType {
            std::unordered_map<std::string, std::function<std::any(std::any)>> methods;
        
            template<typename Ret, typename Arg>
            void setMethod(const std::string& name, std::function<Ret(Arg)> fn) {
                methods[name] = [fn](std::any arg) -> std::any {
                    return fn(std::any_cast<Arg>(arg));
                };
            }
        
            template<typename Ret, typename Arg>
            Ret call(const std::string& name, Arg arg) {
                auto it = methods.find(name);
                if (it != methods.end()) {
                    return std::any_cast<Ret>(it->second(arg));
                }
                throw std::runtime_error("Method not found");
            }
        };
    
        class resource{
            public:
            std::map<std::string, std::shared_ptr<ResourceMetadataBase>> data;
            ResourceID id;
            ResourceType type;

            ~resource(){
            }

            std::shared_ptr<BoolResourceMetadata> set_bool_data(const std::string& key,bool& val){
                auto ptr = std::make_shared<BoolResourceMetadata>(key,val);
                this->data[key] = ptr;
                return ptr;
            }
            std::shared_ptr<StringResourceMetadata> set_str_data(const std::string& key,std::string& val){
                auto ptr = std::make_shared<StringResourceMetadata>(key,val);
                this->data[key] = ptr;
                return ptr;
            }
            std::shared_ptr<IntResourceMetadata> set_int_data(const std::string& key,int& val){
                auto ptr = std::make_shared<IntResourceMetadata>(key,val);
                this->data[key] = ptr;
                return ptr;
            }

            bool get_bool_data(const std::string& key) {
                auto it = data.find(key);
                if (it != data.end()) {
                    auto ptr = std::dynamic_pointer_cast<BoolResourceMetadata>(it->second);
                    if (ptr) return ptr->data;
                }
                throw std::runtime_error("Bool metadata not found or wrong type: " + key);
            }
        
            std::string get_str_data(const std::string& key) {
                auto it = data.find(key);
                if (it != data.end()) {
                    auto ptr = std::dynamic_pointer_cast<StringResourceMetadata>(it->second);
                    if (ptr) return ptr->data;
                }
                throw std::runtime_error("String metadata not found or wrong type: " + key);
            }
        
            int get_int_data(const std::string& key) {
                auto it = data.find(key);
                if (it != data.end()) {
                    auto ptr = std::dynamic_pointer_cast<IntResourceMetadata>(it->second);
                    if (ptr) return ptr->data;
                }
                throw std::runtime_error("Int metadata not found or wrong type: " + key);
            }
        };
    }//core
    
    
    class file_resource:public core::resource{

        BL::FileSink fsink;

        public:
        file_resource(std::string filePath):fsink(filePath,50,10000,"",true){
          id.basic_id = filePath;
          id.backend = BoltID::randomBoltID(1);
          fsink.enable_rotate = false;
        };

        bool write(const std::string& _msg){
            fsink<<_msg;
            return true;
        }
        void flush(){
            fsink<<FSINK;
        }

        CharStream<char> setup(){
            std::ifstream ifs(id.basic_id);
            if (!ifs) return CharStream<char>{{}};
            auto read = std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            CharUtils::CharStream<char> stream = CharUtils::Default_char_import(read);
            return stream;
        }
        bool deleteFile(){
            fsink.closeStream();
            return std::remove(id.basic_id.c_str())==0;
        }
        void openFile(const char* mode){
            fsink.open(mode);
        }
        void closeFile(){
            fsink.closeStream();
        }
    };
    class directory_resource : public core::resource {
        std::vector<directory_resource> __subs, __sups;
        std::vector<file_resource> __subfiles;
    
    public:
        directory_resource(const std::string& dirPath) {
            id.basic_id = dirPath;
            id.backend = BoltID::randomBoltID(1);
        }
    
        void create(){
            std::filesystem::create_directory(id.basic_id);
        }

        void setupFiles() {
            namespace fs = std::filesystem;
        
            if(!__subfiles.empty()) __subfiles.clear();
        
            if (!fs::exists(id.basic_id)) return;
        
            for (const auto& entry : fs::directory_iterator(id.basic_id)) {
                if (entry.is_regular_file()) {
                    __subfiles.push_back(file_resource(entry.path().string()));
                }
            }
        }
        void setup() {

            if(!__subs.empty()) __subs.clear();
            if(!__sups.empty()) __sups.clear();

            namespace fs = std::filesystem;
    
            if (!fs::exists(id.basic_id)) return;
    
            fs::path path(id.basic_id);
            fs::path parent = path.parent_path();
    
            if (!parent.empty() && fs::exists(parent)) {
                __sups.push_back(directory_resource(parent.string()));
            }
    
            for (const auto& entry : fs::directory_iterator(id.basic_id)) {
                if (entry.is_directory()) {
                    __subs.push_back(directory_resource(entry.path().string()));
                }
            }

            setupFiles();
        }
    
        directory_resource& mkdir(const std::string& name){
            std::filesystem::path path = std::filesystem::path(id.basic_id) / name;
            std::filesystem::create_directory(path);
            auto realdir = directory_resource(path.string());
            __subs.push_back(realdir);
            return __subs.back();
        };
        file_resource& mkfile(const std::string& name){
            std::filesystem::path path =std::filesystem::path(id.basic_id) / name;
            file_resource fres {path.string()};
            __subfiles.push_back(fres);
            return __subfiles.back();
        };

        directory_resource sub(const std::string& name) {
            for (auto& d : __subs) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    return d;
            }
            return directory_resource((std::filesystem::path(id.basic_id) / name).string());
        }
        file_resource subfile(const std::string& name) {
            for (auto& d : __subfiles) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    return d;
            }
            return file_resource((std::filesystem::path(id.basic_id) / name).string());
        }
    
        directory_resource sup(const std::string& name) {
            for (auto& d : __sups) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    return d;
            }
            return directory_resource(std::filesystem::path(id.basic_id).parent_path().string());
        }
    
        std::vector<directory_resource> subs(const std::string& name = "") {
            if (name.empty()) return __subs;
            std::vector<directory_resource> filtered;
            for (auto& d : __subs) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    filtered.push_back(d);
            }
            return filtered;
        }
        std::vector<file_resource> subfiles(const std::string& name = "") {
            if (name.empty()) return __subfiles;
            std::vector<file_resource> filtered;
            for (auto& d : __subfiles) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    filtered.push_back(d);
            }
            return filtered;
        }

    
        std::vector<directory_resource> sups(const std::string& name = "") {
            if (name.empty()) return __sups;
            std::vector<directory_resource> filtered;
            for (auto& d : __sups) {
                if (std::filesystem::path(d.id.basic_id).filename() == name)
                    filtered.push_back(d);
            }
            return filtered;
        }
    };
}

#endif // BRAIN_H