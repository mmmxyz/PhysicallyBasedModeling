#!/bin/bash

if [ $# -lt 2 ]; then
	echo "引数が必要だよ"
	exit 1
fi

outShaderPath="$1/ShaderBinary"
echo "ShaderBinary Directory: $outShaderPath"

SLANGC="$2"

if [ ! -d "outShaderPath" ]; then
	echo "mkdir $outShaderPath"
	mkdir -p "$outShaderPath"
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

"$SLANGC/slangc" -target spirv -stage vertex "$SCRIPT_DIR/test.slang" -o "$outShaderPath/test.vert.spv" -entry vertexMain -lang slang
"$SLANGC/slangc" -target spirv -stage fragment "$SCRIPT_DIR/test.slang" -o "$outShaderPath/test.frag.spv" -entry fragmentMain -lang slang
