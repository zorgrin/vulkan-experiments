mkdir build
cd build
cmake "-G" "Visual Studio 15 Win64" "-DGLSLANG_VALIDATOR=%VK_SDK_PATH%\bin\glslangValidator" "-DVULKAN_LIBRARY_DIR=%VK_SDK_PATH%\bin" "-DVULKAN_INCLUDE_DIR=%VK_SDK_PATH%\include" ..\src
cd ..