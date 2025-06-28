cmake_minimum_required(VERSION 3.31)

project(Omnix VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_COMPILE_FEATURES True)

set(CMAKE_BUILD_TYPE Debug)  
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -O0 -g3")

add_subdirectory(CakilganCore)
add_subdirectory(glad)
add_subdirectory(max)
add_subdirectory(brain)
add_subdirectory(freetype)
add_subdirectory(box2d)
add_subdirectory(glm)


file(GLOB TESTS "test/*.cpp")
file(GLOB SOURCES "src/*.cpp")
file(GLOB CORE_INCLUDES "include/*.h")


add_library(OmnixLib STATIC ${SOURCES})
target_link_libraries(OmnixLib PRIVATE CakilganCore glad max brain Thunder2D freetype box2d glm)
target_include_directories(OmnixLib PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
${CMAKE_CURRENT_SOURCE_DIR}/CakilganCore/include
${CMAKE_CURRENT_SOURCE_DIR}/glad/include
${CMAKE_CURRENT_SOURCE_DIR}/max/include
${CMAKE_CURRENT_SOURCE_DIR}/brain/include
${CMAKE_CURRENT_SOURCE_DIR}/Thunder2D/include
${CMAKE_CURRENT_SOURCE_DIR}/freetype/include
${CMAKE_CURRENT_SOURCE_DIR}/box2d/include/box2d
${CMAKE_CURRENT_SOURCE_DIR}/glm/glm
)

add_subdirectory(Thunder2D)

add_executable(OmnixTests
    ${TESTS}
)
target_link_libraries(OmnixTests PRIVATE OmnixLib)


# Test çalıştırma modu için option
option(RUN_ALL_TESTS "Force run all tests even if SET_ONLY_TEST is defined" OFF)

file(GLOB_RECURSE ALL_TEST_FILES 
    "*/test.cpp"
    "*/tests.cpp" 
    "*_test.cpp"
    "*_tests.cpp"
    "test/*.cpp"
    "tests/*.cpp"
)

# Test dosyalarını filtrele (main fonksiyonu olan)
set(VALID_TEST_FILES)
set(PRIORITY_TEST_FILE "")

foreach(test_file ${ALL_TEST_FILES})
    file(READ ${test_file} file_content)
    string(FIND "${file_content}" "int main(" has_main)
    
    if(NOT has_main EQUAL -1)
        # SET_ONLY_TEST define'ı var mı kontrol et
        string(FIND "${file_content}" "#define SET_ONLY_TEST" has_priority_flag)
        
        if(NOT has_priority_flag EQUAL -1)
            set(PRIORITY_TEST_FILE ${test_file})
            message(STATUS "Priority test found: ${test_file}")
        endif()
        
        # Tüm geçerli test dosyalarını listeye ekle
        list(APPEND VALID_TEST_FILES ${test_file})
    endif()
endforeach()

# Test çalıştırma stratejisini belirle
if(RUN_ALL_TESTS)
    set(TESTS_TO_RUN ${VALID_TEST_FILES})
    message(STATUS "RUN_ALL_TESTS option enabled - building ALL tests")
    
elseif(NOT PRIORITY_TEST_FILE STREQUAL "")
    set(TESTS_TO_RUN ${PRIORITY_TEST_FILE})
    message(STATUS "SET_ONLY_TEST detected - building only priority test")
    
else()
    set(TESTS_TO_RUN ${VALID_TEST_FILES})
    message(STATUS "No priority test found - building all tests")
endif()

# Test executable'larını oluştur
set(TEST_EXECUTABLES)
foreach(test_file ${TESTS_TO_RUN})
    file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${test_file})
    string(REPLACE "/" "_" exe_name ${rel_path})
    string(REPLACE ".cpp" "" exe_name ${exe_name})
    
    add_executable(${exe_name} ${test_file})
    target_link_libraries(${exe_name} PRIVATE OmnixLib)
    
    list(APPEND TEST_EXECUTABLES ${exe_name})
    message(STATUS "Test executable: ${rel_path} -> ${exe_name}")
endforeach()

# Windows batch script oluştur (sadece test varsa)
if(TEST_EXECUTABLES AND WIN32)
    set(BATCH_CONTENT "@echo off\n")
    set(BATCH_CONTENT "${BATCH_CONTENT}echo ========================================\n")
    set(BATCH_CONTENT "${BATCH_CONTENT}echo           RUNNING TESTS\n")
    set(BATCH_CONTENT "${BATCH_CONTENT}echo ========================================\n")
    set(BATCH_CONTENT "${BATCH_CONTENT}echo.\n")
    
    foreach(exe_name ${TEST_EXECUTABLES})
        # Debug/Release klasörü için path kontrolü
        set(BATCH_CONTENT "${BATCH_CONTENT}echo [TEST] Running ${exe_name}...\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}if exist \"build\\Debug\\${exe_name}.exe\" (\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}    build\\Debug\\${exe_name}.exe\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}) else if exist \"build\\Release\\${exe_name}.exe\" (\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}    build\\Release\\${exe_name}.exe\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}) else if exist \"build\\${exe_name}.exe\" (\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}    build\\${exe_name}.exe\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}) else (\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}    echo ERROR: ${exe_name}.exe not found!\n")
        set(BATCH_CONTENT "${BATCH_CONTENT})\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}echo.\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}echo ----------------------------------------\n")
        set(BATCH_CONTENT "${BATCH_CONTENT}echo.\n")
    endforeach()
    
    set(BATCH_CONTENT "${BATCH_CONTENT}echo All tests completed!\n")
    set(BATCH_CONTENT "${BATCH_CONTENT}pause\n")
    
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_tests.bat "${BATCH_CONTENT}")
    message(STATUS "Created batch file: run_tests.bat")
endif()

# Test sayısını göster
list(LENGTH TEST_EXECUTABLES test_count)
message(STATUS "Total test executables: ${test_count}")