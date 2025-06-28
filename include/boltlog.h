#ifndef BOLTLOG_H
#define BOLTLOG_H
#include "BoltID.h"
#include "string_utils.h"
#include "test_utils.h"
#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string.h>
#include <mutex>
#include <xstring>
#include <chrono>

#define BL_C_RESET "\033[0m"
#define BL_C_(a) "\033["+std::to_string(a)+"m"
#define DETAIL_BL_C_(foreground,background) "\033["+std::to_string(foreground)+";"+std::to_string(background)+"m"
#define BL_SURROUND(str, prefix, suffix) std::string(prefix) + str + std::string(suffix)

#define BL_COLORIZE(STR,COLOR_int) BL_SURROUND(STR,BL_C_(COLOR_int),BL_C_RESET)
#define DETAIL_BL_COLORIZE(STR,COLOR_int_fore,COLOR_int_back) BL_SURROUND(STR,DETAIL_BL_C_(COLOR_int_fore,COLOR_int_back),BL_C_RESET)



namespace BL{
    struct flog{};
    struct fsink{};
    const static flog _flog{};
    const static fsink _sink{};
   namespace Helper{
       template<typename T>
         std::string to_string_helper(const T& value) {
             if constexpr (std::is_arithmetic_v<T>) {
                 return std::to_string(value);
             } else if constexpr (std::is_pointer_v<T>) {
                 if (value == nullptr) return "nullptr";
                 std::ostringstream oss;
                 oss << value;
                 return oss.str();
             } else {
                 std::ostringstream oss;
                 oss << value;
                 return oss.str();
             }
         }
         inline std::string removeColorCodes(const std::string& input) {
             static const std::regex ansiRegex(R"(\x1B\[[0-9;]*m)");
             return std::regex_replace(input, ansiRegex, "");
         }
         inline std::string to_string_helper(const std::string& value) {
             return value;
         }
         inline std::string to_string_helper(const char* value) {
             return value ? std::string(value) : "nullptr";
         }
         inline std::string resultcx(bool res){
             return res  ? "{cx:32#-SUCCESS-}" : "{cx:31#-FAIL-}";
         }
         inline std::string get_current_thread_ID() {
           std::stringstream ss;
           ss << std::this_thread::get_id();
           return ss.str();
         }
         inline std::mutex& global_log_mutex() {
              static std::mutex mtx;
              return mtx;
         }
         inline std::string removeBrackets(std::string& input, char bracket1 = '(', char bracket2 = ')') {
             if (input.size() >= 2 && input.front() == bracket1 && input.back() == bracket2) {
                 return input.substr(1, input.size() - 2);
             }
             return input;
         }
         inline std::string replaceAll(const std::string& original, const std::string& from, const std::string& to) {
          if (from.empty()) return original;
      
          std::string result = original;
          size_t start_pos = 0;
      
          while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
              result.replace(start_pos, from.length(), to);
              start_pos += to.length(); 
          }
      
          return result;
      }
          inline std::string formatDate(const std::string& format) {
              using namespace std::chrono;
              
              auto now = system_clock::now();
              time_t now_time_t = system_clock::to_time_t(now);
              
              tm ltm;
              localtime_s(&ltm, &now_time_t);
          
              auto now_us = duration_cast<microseconds>(now.time_since_epoch()) % 1'000'000;
          
              std::string start = std::string(format);
              std::ostringstream us_oss;
              us_oss << std::setw(6) << std::setfill('0') << now_us.count();
          
              start = replaceAll(start, "%MS", us_oss.str());
      
              std::ostringstream oss;
              oss << std::put_time(&ltm, start.c_str());
              
              return oss.str();
          }
          inline std::string getCurrentTimestamp() {
              return formatDate("%Y-%m-%d %H:%M:%S");
          }
          inline bool shouldLogAllMatch(const std::vector<std::string>& tags, const std::unordered_set<std::string>& allowed_tags) {
              for (const auto& tag : tags) {
                  if (!allowed_tags.count(tag))
                      return false; 
              }
              return true; 
          }
          inline int countChar(const char &refc,const std::string& ref){
            int ret = 0;
            for(auto a:ref){
                if(a==refc){
                    ret++;
                }
            }
            return ret;
          }
   };
struct LogMetadata {
std::pmr::string msg;
std::pmr::unordered_map<std::pmr::string, std::pmr::string> data;
std::pmr::unordered_map<std::pmr::string, std::vector<std::pmr::string>> flags;
std::vector<std::string> tags;
LogMetadata(const std::pmr::string& msg = {},std::pmr::memory_resource* res = std::pmr::get_default_resource())
    : msg(msg,res), data(res),flags(res) {}
    
inline std::pmr::string& operator[](const std::pmr::string& key) {
    return data[key];
}

inline const std::pmr::string& operator[](const std::pmr::string& key) const {
    static const std::pmr::string empty{};
    auto it = data.find(key);
    return it != data.end() ? it->second : empty;
}
};
template <typename MessageType>
struct Sink{
    virtual void recieve(const MessageType& msg) = 0;
    virtual ~Sink() = default;
    Sink& operator<<(const MessageType& msg) {
        recieve(msg);
        return *this;
    }
    virtual Sink& operator<<(const LogMetadata& metadata)= 0;
    virtual Sink& operator<<(const fsink& flush)= 0;
    virtual Sink<std::string>& operator<<(const std::vector<LogMetadata>& metadata) = 0;
};
struct ConsoleSink:Sink<std::string>{
    int flush_interval = 50,flush_counter = 0;
    public:
    ConsoleSink(int flush_interval=50):flush_interval(flush_interval){}
    inline void recieve(const std::string& msg) override{
        fwrite(msg.c_str(), sizeof(char), msg.size(), stdout);
        if (++flush_counter % flush_interval == 0) {
            fflush(stdout); 
        }
    } 
    Sink<std::string>& operator<<(const LogMetadata& metadata)override {
        this->recieve(std::string(metadata.msg));
        return *this;
    }
    Sink<std::string>& operator<<(const std::vector<LogMetadata>& metadata)override {
        for(auto &ref:metadata){
            *this<<ref;
        }
        return *this;
    }
     Sink& operator<<(const fsink& flush) override{
        if (++flush_counter % flush_interval == 0) {
            fflush(stdout); 
        }
        return *this;
    }
    
};
struct FileSink:Sink<std::string>{
    private:
    FILE* file;
    int flush_interval = 50,flush_counter;
    int sink_volume,sink_counter = 0;
    std::string fpath;
    public:
    bool enable_rotate = true;
    std::string __ix;
    FileSink(const std::string& file_path,int flush_interval=50,int sink_volume=10000,const std::string& __ix = ".blog",bool filemode = false):file(nullptr),flush_interval(flush_interval),sink_volume(sink_volume),flush_counter(0),sink_counter(0){
        fpath = file_path;
        this->__ix = __ix;
        if(!filemode)
         open("w");
    }
    inline void open(const char* mode){
         std::string _f = std::string(fpath)+__ix;
         fopen_s(&file,_f.c_str(), mode);
    }
    inline void set_flush_interval(const int& v0){
        flush_interval = v0;
    }
    inline void set_sink_volume(const int& v0){
        sink_volume = v0;
    }
    std::string heap;
    inline void recieve(const std::string& msg) override{
        
        auto nocolor = BL::Helper::removeColorCodes(msg);
        heap+=nocolor;
    }
    inline void closeStream(){
        if (file) {
            fflush(file); 
            fclose(file);
            file = nullptr;
        }
    }

    inline void rotate(const std::string& _old,const std::string& _new){
        auto r= std::string("---LOGGER."+_old+" close "+Helper::getCurrentTimestamp()+"---");
        fwrite(r.c_str(), sizeof(char), r.size(), file);
        closeStream();
        fopen_s(&file, std::string(_new+Helper::formatDate("%Y_%m_%d_%H_%M_%S")+".blog").c_str(), "w");
    }
    void flush(){
        if (!file) return;
        if(flush_counter%flush_interval == 0){
            fflush(file);
        }
        if (flush_counter >= sink_volume) {
            sink_counter++;
            if(enable_rotate){
                rotate(fpath, fpath + "_" + std::to_string(sink_counter)+"_");
                flush_counter = 0;
            }
        }
        fwrite(heap.c_str(), sizeof(char), heap.size(), file);
        heap.clear();
        flush_counter++;
    }
    Sink<std::string>& operator<<(const fsink& flsh) override{
        flush();
        return *this;
    }
    Sink<std::string>& operator<<(const std::string& _m){
        this->recieve(_m);
        return *this;
    }
    Sink<std::string>& operator<<(const LogMetadata& metadata)override {
        this->recieve(std::string(metadata.msg));
        return *this;
    }
    Sink<std::string>& operator<<(const std::vector<LogMetadata>& metadata)override {
        for(auto &ref:metadata){
            *this<<ref;
        }
        return *this;
    }
    ~FileSink() {
        if (file) {
            fflush(file); 
            fclose(file);
        }
    }
};
struct ASyncFileSink:FileSink{
    private:
    std::queue<LogMetadata> _data;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> running = true;
    std::thread worker;
    public:
    ASyncFileSink(const std::string& fpath,int flush_interval=50,int sink_volume=10000):FileSink(fpath,flush_interval,sink_volume){
         worker = std::thread([this] {
            while (running || !isQueueEmpty()) {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this] { return !_data.empty() || !running; });

                while (!_data.empty()) {
                    LogMetadata meta = std::move(_data.front());
                    _data.pop();
                    lock.unlock();
                     FileSink::operator<<(meta);  
                    lock.lock();
                }
            }
            
        });
        
    }
    Sink<std::string>& operator<<(const fsink& flsh) override{
        flush();
        return *this;
    }
    
    Sink<std::string>& operator<<(const LogMetadata& metadata)override {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _data.push(std::move(metadata));
        }
        _cv.notify_one();
        return *this;
    }
    Sink<std::string>& operator<<(const std::vector<LogMetadata>& metadata)override {
        for(auto &ref:metadata){
            *this<<ref;
        }
        return *this;
    }
    void stop() {
        running = false;
        _cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    bool isQueueEmpty() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _data.empty();
    }
    ~ASyncFileSink() {
        stop();
    }
};
struct ASyncConsoleSink:ConsoleSink{
    private:
    std::queue<LogMetadata> _data;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> running = true;
    std::thread worker;
    public:
    ASyncConsoleSink():ConsoleSink(){
         worker = std::thread([this] {
            while (running || !isQueueEmpty()) {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this] { return !_data.empty() || !running; });

                while (!_data.empty()) {
                    LogMetadata meta = std::move(_data.front());
                    _data.pop();
                    lock.unlock();
                     ConsoleSink::operator<<(meta);  
                    lock.lock();
                }
            }
            
        });
        
    }
    Sink<std::string>& operator<<(const fsink& flsh) override{
        return *this;
    }
    
    Sink<std::string>& operator<<(const LogMetadata& metadata)override {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _data.push(std::move(metadata));
        }
        _cv.notify_one();
        return *this;
    }
    Sink<std::string>& operator<<(const std::vector<LogMetadata>& metadata)override {
        for(auto &ref:metadata){
            *this<<ref;
        }
        return *this;
    }
    void stop() {
        running = false;
        _cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    bool isQueueEmpty() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _data.empty();
    }
    ~ASyncConsoleSink() {
        stop();
    }
};
class LogFormatter{
    public:
    std::pmr::unordered_map<std::string_view, std::function<std::string(std::string_view)>> formatterMap;
    LogFormatter(const std::pmr::unordered_map<std::string_view, std::function<std::string(std::string_view)>>& formatterMap):formatterMap(formatterMap){}
    
    inline LogMetadata format(const std::string_view input) {
    LogMetadata meta;
    std::ostringstream output;
    size_t i = 0;
    int braceLevel = 0;
    size_t tokenStart = std::string::npos;

    while (i < input.size()) {
        if (input[i] == '{') {
            if (braceLevel == 0) {
                tokenStart = i;
            }
            braceLevel++;
            i++;
        } else if (input[i] == '}') {
            braceLevel--;
            if (braceLevel == 0 && tokenStart != std::string::npos) {
                size_t tokenEnd = i;
                std::string_view token = input.substr(tokenStart + 1, tokenEnd - tokenStart - 1);
                size_t colon = token.find(':');
                if (colon != std::string::npos) {
                    std::string_view id = token.substr(0, colon);
                    std::string_view val = token.substr(colon + 1);
                    auto it = formatterMap.find(std::string(id));
                    if (it != formatterMap.end()) {
                        std::string rt = it->second(val);
                        if (id == "flag") {
                            meta.flags["flag"].push_back(std::pmr::string(val));
                        }
                        if(id=="tag"){
                            meta.tags.push_back(std::string(rt));
                        }
                        meta[std::string(id).c_str()] = std::string(rt);
                        output << rt;
                    } else {
                        output << "{" << token << "}";
                    }
                } else {
                    output << "{" << token << "}";
                }
                tokenStart = std::string::npos;
                i++;
            } else {
                i++;
            }
        } else {
            if (braceLevel == 0) {
                output << input[i];
            }
            i++;
        }
    }

    meta.msg = output.str();
    return meta;
}

};
class LogBuilder{
    private:
    LogBuilder(){}
    std::string log;
    public:
    std::string build() const{
        return log;
    }
    LogBuilder& literal(std::string_view v){
        log+=v;
        return *this;
    }
    LogBuilder& newline(){
        log+="\n";
        return *this;
    }
    LogBuilder& space(){
        log+=" ";
        return *this;
    }
    LogBuilder& add(std::string_view key,std::string_view value){
        log+="$";
        log+=key;
        log+=":";
        log+=value;
        log+="$";
        return *this;
    }
    static LogBuilder create(){
        return LogBuilder();
    }
};
class AbstractLogger{
    public:
    std::vector<std::shared_ptr<Sink<std::string>>> string_sinks;
    LogFormatter* formatter;
    virtual void log(std::string_view msg) = 0;
};

namespace Default{
    enum LoggerLevel:int{
        INFO = 32,
        WARNING = 33,
        DEBUG = 34,
        ERROR = 31,
        FATAL = 91,
        TRACE = 36,
        LIFECYCLE = 35,
        INVALID = -1
    };
    static int ordinal(LoggerLevel lv){
        switch (lv) {
            case INFO: return 0;
            case LIFECYCLE: return 1;
            case WARNING: return 2;
            case DEBUG: return 3;
            case ERROR: return 4;
            case TRACE: return 5;
            case FATAL: return 6;
            case INVALID: return -100;
        }
    }
    static std::string levelName(const LoggerLevel& level){
        switch (level) {
            case Default::LoggerLevel::INFO :return "INFO";
            case Default::LoggerLevel::WARNING :return "WARNING";
            case Default::LoggerLevel::DEBUG :return "DEBUG";
            case Default::LoggerLevel::ERROR :return "ERROR";
            case Default::LoggerLevel::FATAL :return "FATAL";
            case Default::LoggerLevel::TRACE :return "TRACE";
            default : return "INVALID";
        }
    }
    static LoggerLevel getLevel(std::string_view from){
        if(from=="INFO"){
            return LoggerLevel::INFO;
        }else if (from == "WARNING")
        {
            return LoggerLevel::WARNING;
        }
        else if (from == "DEBUG")
        {
            return LoggerLevel::DEBUG;
        }else if (from == "LIFECYCLE")
        {
            return LoggerLevel::LIFECYCLE;
        }
        else if (from == "ERROR")
        {
            return LoggerLevel::ERROR;
        }
        else if (from == "FATAL")
        {
            return LoggerLevel::FATAL;
        }
        else if (from == "TRACE")
        {
            return LoggerLevel::TRACE;
        }else{
            return LoggerLevel::INVALID;
        }
    }

    struct ScopedTag{
    };
    
    class Logger:public AbstractLogger{
        std::mutex mutex;
        constexpr static std::string_view alloc_flag = "v#%ALLOCATE%";
        constexpr static std::string_view alloc_flag2 = "%ALLOCATE%";
        
        static bool contains_flag(const std::vector<std::pmr::string>& flags, std::string_view needle) {
            return std::find(flags.begin(), flags.end(), needle) != flags.end();
        }
        std::string loggerName;
        BoltID id = BoltID::randomBoltID(1);
        inline LogMetadata flush(){
            LogMetadata metadata = formatter->format(os);
            auto &v = metadata.flags["flag"];
            metadata.data[std::pmr::string("id")] = std::pmr::string(ID().toString());
            if(!globalPrefixs.empty()){
                for(auto prefix : globalPrefixs){
                    metadata.msg.insert(0,prefix+" ");
                    std::string ncprefix = Helper::removeColorCodes(prefix);
                    metadata.data.emplace(std::pmr::string("prefix%"+ncprefix),std::pmr::string(ncprefix));
                }
            }
            if(!globalSuffixs.empty()){
                for(auto suffix : globalSuffixs){
                    metadata.msg.insert(metadata.msg.size()," "+suffix);
                    std::string ncsuffix = Helper::removeColorCodes(suffix);
                    metadata.data.emplace(std::pmr::string("suffix%"+ncsuffix),std::pmr::string(ncsuffix));
                }
            }
            if(contains_flag(v,alloc_flag)){
                allocateds.push_back(metadata);
            }
            if(contains_flag(v,alloc_flag2)){
                allocateds.push_back(metadata);
            }
            
            os.clear();
            return metadata;
        };
        public:
        inline const std::string& name() const{
            return loggerName;
        }
        static std::vector<std::string> globalSuffixs;
        static std::vector<std::string> globalPrefixs;
        std::vector<LogMetadata> allocateds;
        LoggerLevel lvl;
        
        static const std::pmr::unordered_map<std::string_view, std::function<std::string(std::string_view)>> map;
        LogFormatter default_formatter{map};


        Logger(){
            formatter = &default_formatter;
            set_ordinal(INFO);
            loggerName = "ANON";
        }
        Logger(const std::string& loggerName){
            formatter = &default_formatter;
            set_ordinal(INFO);
            this->loggerName = loggerName;
        }
        Logger(LogFormatter* formatter){
            this->formatter = formatter;
            set_ordinal(INFO);
            this->loggerName = "ANON";
        }

        inline void set_ordinal(const LoggerLevel& lvl){
            std::lock_guard<std::mutex> lock(mutex);
            this->lvl = lvl;
        }
        inline BoltID ID(){
            return id;
        }
        std::string os;
        void log(std::string_view msg) override{
                os+=msg;
        }
        Logger& operator<<(const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex);
            log(msg);
            return *this;
        }
        Logger& operator<<(const int& msg) {
            std::lock_guard<std::mutex> lock(mutex);
            log(std::to_string(msg));
            return *this;
        }
        Logger& operator<<(const flog& f) {
            std::lock_guard<std::mutex> lock(mutex);
            auto md = flush();
            std::string lv = BL::Helper::removeColorCodes(std::string(md["lvl"]));
            lv = BL::Helper::removeBrackets(lv,'[',']');

            if(Helper::countChar('\n', std::string(md.msg))>1){
                int loc = md.msg.find_first_of('\n');
                md.msg.insert(loc,BL_COLORIZE("{",34));
                md.msg.append(BL_COLORIZE("}",34));
                md.msg.append("\n");
            }
            
            if(ordinal(getLevel(lv))>=ordinal(lvl)){
               for(auto& sink:string_sinks){
                    *sink<<md;
               } 
            }
            return *this;
        }
        Logger& operator<<(const fsink& f) {
            for(auto &sink:string_sinks){
                 if (auto fsink = std::dynamic_pointer_cast<FileSink>(sink)) {
                     *fsink<<f;
                 }
            }
            return *this;
        }
        inline std::vector<LogMetadata> filterLevel(const LoggerLevel& level){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                std::string lv = BL::Helper::removeColorCodes(std::string(l["lvl"]));
                lv = BL::Helper::removeBrackets(lv,'[',']');
                if(lv==levelName(level)){
                    rtrn.push_back(l);
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterDate(const std::string& start_date,const std::string& end_date){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                std::string lv = BL::Helper::removeColorCodes(std::string(l["datef"]));
                lv = BL::Helper::removeBrackets(lv,'{','}');
                if(lv>start_date &&lv<end_date){
                    rtrn.push_back(l);
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterTag(const std::string& tag){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                for(auto& f:l.tags){
                    auto holder = Helper::removeColorCodes(f);
                    if(holder==tag){
                        rtrn.push_back(l);
                    }
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterTag(const std::unordered_set<std::string>& tag){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                std::string f = std::string(l["tag"]);
                f = Helper::removeColorCodes(f);
                if(tag.contains(f)){
                    rtrn.push_back(l);
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterTagAllEQ(const std::vector<std::string>& tag){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                std::vector<std::string> nocolor{};
                for(auto holder:l.tags){
                    auto _h = Helper::removeColorCodes(holder);
                    nocolor.push_back(_h);    
                }
                if(nocolor==tag){
                    rtrn.push_back(l);
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterPrefix(const std::string& prefix){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                const std::string _pr = "prefix%"+prefix;
                if(l.data.find(std::pmr::string(_pr))!=l.data.end()){
                    if(l.data.at(std::pmr::string(_pr))==std::pmr::string(prefix)){
                        rtrn.push_back(l);
                    }
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterSuffix(const std::string& suffix){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                const std::string _pr = "suffix%"+suffix;
                if(l.data.find(std::pmr::string(_pr))!=l.data.end()){
                    if(l.data.at(std::pmr::string(_pr))==std::pmr::string(suffix)){
                        rtrn.push_back(l);
                    }
                }
            }
            return rtrn;
        }
        inline std::vector<LogMetadata> filterID(const std::string& to_string){
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<LogMetadata> rtrn;
            for(auto l:allocateds){
                if(l.data.find(std::pmr::string("id"))!=l.data.end()){
                    if(l.data.at(std::pmr::string("id"))==std::pmr::string(to_string)){
                        rtrn.push_back(l);
                    }
                }
            }
            return rtrn;
        };
       
        
    };
    

    inline const std::pmr::unordered_map<std::string_view, std::function<std::string(std::string_view)>> BL::Default::Logger::map
    = {
          {
        "datef",
        [](std::string_view input){
           return BL_COLORIZE(BL::Helper::formatDate(std::string(input)), 90);
        }
        },{
        "lvl",
        [](std::string_view input){
            std::string l = std::string(input);
           return BL_COLORIZE(std::string(input), getLevel(Utils::StringUtils::removeBrackets(l,'[',']')));
        }
        },{
        "msg",
        [](std::string_view input){
        return std::string(input);
        }
        },
        {
        "thread",
        [](std::string_view input){
           return BL_COLORIZE(std::string(input), 96);
        }
        },
        {
        "cx",
        [](std::string_view input){
            auto v = Utils::StringUtils::splitByFirst('#', std::string(input));
            return BL_COLORIZE(v[1], std::stoi(v[0]));
        }
        },
        {
        "flag",
        [](std::string_view input){
            std::string val =std::string(input);
            auto vec = Utils::StringUtils::splitByFirst('#',val);
            if(vec.size()>1){
                if(vec[0]=="v"){
                    return std::string("");
                }
            }
            return BL_COLORIZE(std::string(input),36);
        }
        },{
        "tag",
        [](std::string_view input){
            std::string val =std::string(input);
            auto s = Utils::StringUtils::splitByFirst('#', val);
            return BL_COLORIZE(s[1],std::stoi(s[0]));
        }
        },
        {
        "id",
        [](std::string_view input){
            return BL_COLORIZE(std::string(input), 92);
        }
        }
        
        
    };

    
    class HasLogger{
        std::shared_ptr<BL::Default::Logger> _logger;
        public:
        HasLogger(std::vector<std::shared_ptr<Sink<std::string>>> string_sinks,std::string loggerName="ANON"){
            _logger = std::make_shared<BL::Default::Logger>(loggerName);
            for(auto sink:string_sinks){
                _logger->string_sinks.push_back(sink);
            }
            
        }
        virtual std::shared_ptr<BL::Default::Logger> logger(){return _logger;}
    };
};
}



#define LMUTEX(X) \
    { std::lock_guard<std::mutex> _lg(BL::Helper::global_log_mutex()); X; }


#define _LID(A) " {id:"<<A<<"}"
#define LID_(A) "{id:"<<A<<"} "
#define _LID_(A) " {id:"<<A<<"} "

#define LID(A,B) A<<LID_(B)

#define _INFO " {lvl:[INFO]}"
#define _WARNING " {lvl:[WARNING]}"
#define __DEBUG " {lvl:[DEBUG]}"
#define _ERROR " {lvl:[ERROR]}"
#define _FATAL " {lvl:[FATAL]}"
#define _TRACE " {lvl:[TRACE]}"
#define _LIFECYCLE " {lvl:[LIFECYCLE]}"

#define INFO_ "{lvl:[INFO]} "
#define WARNING_ "{lvl:[WARNING]} "
#define DEBUG_ "{lvl:[DEBUG]} "
#define ERROR_ "{lvl:[ERROR]} "
#define FATAL_ "{lvl:[FATAL]} "
#define TRACE_ "{lvl:[TRACE]} "
#define LIFECYCLE_ "{lvl:[LIFECYCLE]} "

#define _INFO_ " {lvl:[INFO]} "
#define _WARNING_ " {lvl:[WARNING]} "
#define _DEBUG_ " {lvl:[DEBUG]} "
#define _ERROR_ " {lvl:[ERROR]} "
#define _FATAL_ " {lvl:[FATAL]} "
#define _TRACE_ " {lvl:[TRACE]} "
#define _LIFECYCLE_ " {lvl:[LIFECYCLE]} "

#define INFO(A) A<<INFO_
#define WARNING(A) A<<WARNING_
#define DEBUG(A) A<<DEBUG_
#define __ERROR(A) A<<ERROR_
#define FATAL(A) A<<FATAL_
#define TRACE(A) A<<TRACE_
#define LIFECYCLE(A) A<<LIFECYCLE_


#define _TIME " {datef:{%Y-%m-%d %H:%M:%S.%MS}}"
#define TIME_ "{datef:{%Y-%m-%d %H:%M:%S.%MS}} "
#define _TIME_ " {datef:{%Y-%m-%d %H:%M:%S.%MS}} "

#define TIME(A) A<<TIME_

#define _CX(color,cx) " {cx:"<<#color<<"#"<<cx<<"}"
#define CX_(color,cx) "{cx:"<<#color<<"#"<<cx<<"} "
#define _CX_(color,cx) " {cx:"<<#color<<"#"<<cx<<"} "

#define CX(A,color,cx) A<<CX_(color,cx)

#define _THREADID " {thread:("+BL::Helper::get_current_thread_ID()+")}"
#define THREADID_ "{thread:("+BL::Helper::get_current_thread_ID()+")} "
#define _THREADID_ " {thread:("+BL::Helper::get_current_thread_ID()+")} "

#define THREADID(A) A<<THREADID_

#define _FLAG_(A) " {flag:%"<<A<<"%} "
#define _FLAG(A) " {flag:%"<<A<<"%}" 
#define FLAG_(A) "{flag:%"<<A<<"%} " 
#define FLAG(A) "{flag:v#%"<<A<<"%}"

#define blALLOCATE "ALLOCATE"

#define _TAG_(A) " {tag:32#"<<A<<"} "
#define _TAG(A) " {tag:32#"<<A<<"}"
#define TAG_(A) "{tag:32#"<<A<<"} "

#define _cTAG_(A,b) " {tag:"<<b<<"#"<<A<<"} "
#define _cTAG(A,b) " {tag:"<<b<<"#"<<A<<"}"
#define cTAG_(A,b) "{tag:"<<b<<"#"<<A<<"} "

#define LOG_INFO_CTX(A,B) INFO(THREADID(LID(TIME(A),B))) 
#define LOG_WARNING_CTX(A,B) WARNING(THREADID(LID(TIME(A),B)))
#define LOG_DEBUG_CTX(A,B) DEBUG(THREADID(LID(TIME(A),B)))
#define LOG_ERROR_CTX(A,B) __ERROR(THREADID(LID(TIME(A),B)))
#define LOG_FATAL_CTX(A,B) FATAL(THREADID(LID(TIME(A),B)))
#define LOG_TRACE_CTX(A,B) TRACE(THREADID(LID(TIME(A),B)))
#define LOG_LIFECYCLE_CTX(A,B) LIFECYCLE(THREADID(LID(TIME(A),B)))


#define LOG_INFO_BASE(A) INFO(THREADID(LID(TIME(A),"["+A.name()+"]"))) 
#define LOG_WARNING_BASE(A) WARNING(THREADID(LID(TIME(A),"["+A.name()+"]")))
#define LOG_DEBUG_BASE(A) DEBUG(THREADID(LID(TIME(A),"["+A.name()+"]")))
#define LOG_ERROR_BASE(A) __ERROR(THREADID(LID(TIME(A),"["+A.name()+"]")))
#define LOG_FATAL_BASE(A) FATAL(THREADID(LID(TIME(A),"["+A.name()+"]")))
#define LOG_TRACE_BASE(A) TRACE(THREADID(LID(TIME(A),A.ID().toString())))
#define LOG_LIFECYCLE_BASE(A) LIFECYCLE(THREADID(LID(TIME(A),"["+A.name()+"]")))


#define GET_MACRO(_1,_2,NAME,...) NAME
#define LOG_INFO(...) GET_MACRO(__VA_ARGS__, LOG_INFO_CTX, LOG_INFO_BASE)(__VA_ARGS__)
#define LOG_WARNING(...) GET_MACRO(__VA_ARGS__, LOG_WARNING_CTX, LOG_WARNING_BASE)(__VA_ARGS__)
#define LOG_DEBUG(...) GET_MACRO(__VA_ARGS__, LOG_DEBUG_CTX, LOG_DEBUG_BASE)(__VA_ARGS__)
#define LOG_ERROR(...) GET_MACRO(__VA_ARGS__, LOG_ERROR_CTX, LOG_ERROR_BASE)(__VA_ARGS__)
#define LOG_FATAL(...) GET_MACRO(__VA_ARGS__, LOG_FATAL_CTX, LOG_FATAL_BASE)(__VA_ARGS__)
#define LOG_TRACE(...) GET_MACRO(__VA_ARGS__, LOG_TRACE_CTX, LOG_TRACE_BASE)(__VA_ARGS__)
#define LOG_LIFECYCLE(...) GET_MACRO(__VA_ARGS__, LOG_LIFECYCLE_CTX, LOG_LIFECYCLE_BASE)(__VA_ARGS__)

#define implTRACE(logger,OP, A, B, FILE, LINE) LOG_TRACE(logger)<<BL::Helper::resultcx(A OP B)<<" "<<#A<<" "<<#OP<<" "<<#B<<" ("<<BL::Helper::to_string_helper(A)<<" ?= "<<BL::Helper::to_string_helper(B)<<")"<<" at "<<std::string{FILE}<<":"<<std::to_string(LINE)
#define implLOGIC(OP,A,B,FILE,LINE) BL::Helper::resultcx(A OP B)<<" "<<#A<<" "<<#OP<<" "<<#B<<" ("<<BL::Helper::to_string_helper(A)<<" ?= "<<BL::Helper::to_string_helper(B)<<")"<<" at "<<std::string{FILE}<<":"<<std::to_string(LINE)

#define EQ(A,B) implLOGIC(==,A,B,__FILE__,__LINE__)
#define NE(A,B) implLOGIC(!=,A,B,__FILE__,__LINE__)
#define LT(A,B) implLOGIC(<,A,B,__FILE__,__LINE__)
#define GT(A,B) implLOGIC(>,A,B,__FILE__,__LINE__)
#define ITRUE(A,B) implLOGIC(==,A,true,__FILE__,__LINE__)
#define IFALSE(A,B) implLOGIC(==,A,false,__FILE__,__LINE__)
#define INULL(A,B) implLOGIC(==,A,nullptr,__FILE__,__LINE__)
#define INNULL(A,B) implLOGIC(!=,A,nullptr,__FILE__,__LINE__)

#define eqTRACE(logger,A, B) implTRACE(logger,==, A, B, __FILE__, __LINE__)
#define neTRACE(logger,A, B) implTRACE(logger,!=, A, B, __FILE__, __LINE__)
#define ltTRACE(logger,A, B) implTRACE(logger,<, A, B, __FILE__, __LINE__)
#define gtTRACE(logger,A, B) implTRACE(logger,>, A, B, __FILE__, __LINE__)
#define trueTRACE(logger,A) implTRACE(logger,==, A, true, __FILE__, __LINE__)
#define falseTRACE(logger,A) implTRACE(logger,==, A, false, __FILE__, __LINE__)
#define nullTRACE(logger,A) implTRACE(logger,==, A, nullptr, __FILE__, __LINE__)
#define nnullTRACE(logger,A) implTRACE(logger,!=, A, nullptr, __FILE__, __LINE__)

#define ADD_PREFIX(A) BL::Default::Logger::globalPrefixs.push_back(A);
#define POP_PREFIX() BL::Default::Logger::globalPrefixs.pop_back();

#define ADD_SUFFIX(A) BL::Default::Logger::globalSuffixs.push_back(A);
#define POP_SUFFIX() BL::Default::Logger::globalSuffixs.pop_back();


#define FLOG BL::_flog
#define FSINK BL::_sink
#define blFLUSH FLOG<<FSINK
#define blENDL "\n"<<blFLUSH

#endif // BOLTLOG_H