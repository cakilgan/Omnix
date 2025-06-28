#include <max.h>
#include <numbers>


float max::degtorad(const float &deg){
    return deg * std::numbers::pi / 180.0f;
}
float max::radtodeg(const float &rad){
    return rad * 180.0f / std::numbers::pi;
}

