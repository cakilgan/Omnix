@echo off
echo ========================================
echo           RUNNING TESTS
echo ========================================
echo.
echo [TEST] Running max_tests...
if exist "build\Debug\max_tests.exe" (
    build\Debug\max_tests.exe
) else if exist "build\Release\max_tests.exe" (
    build\Release\max_tests.exe
) else if exist "build\max_tests.exe" (
    build\max_tests.exe
) else (
    echo ERROR: max_tests.exe not found!
)
echo.
echo ----------------------------------------
echo.
echo [TEST] Running test_tests...
if exist "build\Debug\test_tests.exe" (
    build\Debug\test_tests.exe
) else if exist "build\Release\test_tests.exe" (
    build\Release\test_tests.exe
) else if exist "build\test_tests.exe" (
    build\test_tests.exe
) else (
    echo ERROR: test_tests.exe not found!
)
echo.
echo ----------------------------------------
echo.
echo All tests completed!
pause
