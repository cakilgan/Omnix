#include "include/max.h"
#include "test_utils.h"
#include "time_utils.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
TEST(mxTest1){    
    return BoltTestResult::CALCULATED;
};
int main(){
    COLORIZED_MODE = true;
    TIME_PROFILER_IS_ON = true;


    BOLT_TEST(maxTest1,"test 1",mxTest1);

    std::ofstream fileStream{"profilerResults__max.json"};
    runTests(std::cout,fileStream);
    fileStream.close();


    return 0;
}