# DirectX
This project will load, display and rotates a model. It runs a given time and generates a benchmark data file in the subfolder /data.

# Getting the project running
This readme currently assumes you're using a 64-bit Windows machine.

The project accept 0 or 4 args. Where with 0 args the default values will be taken.
1. reference naam for the generated benchmark file.
2. runtime of the benchmark.
3. model path.
4. texture path.


If the project won't boot, double check the spelling and cases from your model.

If you get following error Error X4583	semantic 'SV_PrimitiveID' unsupported on ps_4_0_level_9_3
Please specify pixelShader properties -> HLSL compiler -> General -> Shader model -> Shader Model 4 (/4_0)

# Credits
Huge thanks to Chili Tomato Noodle, go check him out. -> https://www.youtube.com/user/ChiliTomatoNoodle
