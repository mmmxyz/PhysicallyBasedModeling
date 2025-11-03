
@echo off

set outShaderPath=%1/ShaderBinary
echo "ShaderBinary Directory:" "%outShaderPath%"

if not exist "%outShaderPath%" (
    echo "mkdir" "%outShaderPath%"
    mkdir "%outShaderPath%"
)

%VULKAN_SDK%\Bin\slangc.exe -target spirv -stage vertex %~dp0\test.slang -o %outShaderPath%\test.vert.spv -entry vertexMain
%VULKAN_SDK%\Bin\slangc.exe -target spirv -stage fragment %~dp0\test.slang -o %outShaderPath%\test.frag.spv -entry fragmentMain