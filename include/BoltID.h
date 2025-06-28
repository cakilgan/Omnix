#ifndef BOLT_ID_H
#define BOLT_ID_H

#include "string_utils.h"
#include <random>
#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <xstring>

//! recommended format is 3 :  []-[]-[]
class BoltID {
    std::vector<std::vector<uint8_t>> datas{};
public:
    static std::vector<uint8_t> newWord(uint64_t seed){
        if (seed == 0) return {0};
          std::vector<uint8_t> result;
        while (seed > 0) {
        result.insert(result.begin(), static_cast<uint8_t>(seed % 256));
        seed /= 256;
        }
        return result;
    }
    static std::string wordToString(const std::vector<uint8_t>& word){
       std::ostringstream oss;
       for (uint8_t b : word) {
           if (b >= 32 && b < 127 && b != '$' && b!='-'&&b!='{'&&b!='}') {
               oss << static_cast<char>(b);
           } else if (b == 36) {
               oss << "$DOLLAR$";
           }  else if (b == 45) {
               oss << "$DASH$";
           }else if (b == 123) {
               oss << "$LCBRACES$";
           }  else if (b == 125) {
               oss << "$RCBRACES$";
           }else {
               oss << "$" << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(b) << "$";
           }
       }
       return oss.str();    
    }
    static std::vector<uint8_t> stringToWord(const std::string& str){
        std::vector<uint8_t> result;
        for (size_t i = 0; i < str.size();) {
            if (str[i] == '$') {
                size_t end = str.find('$', i + 1);
                if (end == std::string::npos) throw std::runtime_error("Malformed dollar sequence");
                std::string token = str.substr(i + 1, end - i - 1);
                if (token == "DOLLAR") {
                    result.push_back('$');
                } else if (token == "DASH") {
                    result.push_back('-');
                } else if (token == "LCBRACES") {
                    result.push_back('{');
                }else if (token == "RCBRACES") {
                    result.push_back('}');
                }else {
                     int byte_val = std::stoi(token, nullptr, 16);
                    if (byte_val < 0 || byte_val > 255)
                        throw std::runtime_error("Invalid byte value");
                    result.push_back(static_cast<uint8_t>(byte_val));
                }
                i = end + 1;
            } else {
                result.push_back(static_cast<uint8_t>(str[i]));
                ++i;
            }
        }
        return result;
    } 

    static BoltID randomBoltID(int count = 3){
         static std::random_device rd;
         static std::mt19937_64 gen(rd()); 
         BoltID id{};
         for (int i = 0; i < count; i++)
         {
             id.addWord(gen());
         }
         return id;
    }
    static BoltID fromString(std::string& from){
       BoltID id{};
        std::vector<std::string> words = Utils::StringUtils::splitByDash(from);
       for (int i = 0; i < words.size(); i++)
       {
          id.addWord(id.stringToWord(Utils::StringUtils::removeBrackets(words[i],'[',']')));
       }
       return id;
    }



    inline void addWord(uint64_t word){
        datas.push_back(newWord(word));
    }
    inline void addWord(std::vector<uint8_t> dataword){
        datas.push_back(dataword);
    }

    inline std::vector<std::vector<uint8_t>> getDatas() const{
        return datas;
    }
    bool operator==(const BoltID& other) const {
        return datas == other.datas;
    }


    bool operator!=(const BoltID& other) const {
        return !(*this == other);
    }

    inline std::string toString() const {
     std::ostringstream os;
     for (int i = 0; i < getDatas().size(); i++)
    {
        auto data = getDatas()[i];
        os<<"["<<BoltID::wordToString(data)<<"]";
        if(i!=getDatas().size()-1){
            os<<"-";
        }
    }
    return os.str();
    };



};
inline std::ostream& operator<<(std::ostream& os, const BoltID& bolt) {
    for (int i = 0; i < bolt.getDatas().size(); i++)
    {
        auto data = bolt.getDatas()[i];
        os<<"["<<BoltID::wordToString(data)<<"]";
        if(i!=bolt.getDatas().size()-1){
            os<<"-";
        }
    }
    return os;
}

namespace std {
    template<>
    struct hash<BoltID> {
        std::size_t operator()(const BoltID& id) const noexcept {
            return std::hash<std::string>()(id.toString());
        }
    };
}

#endif // BOLT_ID_H

