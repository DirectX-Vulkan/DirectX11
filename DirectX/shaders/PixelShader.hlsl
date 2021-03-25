/*
cbuffer CBuf
{
    float4 face_colors[6];
};

struct Input {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};


float4 main(Input input) : SV_TARGET{
    return input.color;
}


float4 main(uint tid : SV_PrimitiveID) : SV_Target
{
    return face_colors[tid/2];
}
*/

/////////////
// GLOBALS //
/////////////
Texture2D shaderTexture;
SamplerState SampleType;

//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXTCOORD;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
    float4 textureColor;
    
    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
    textureColor = shaderTexture.Sample(SampleType, input.tex);

    return textureColor;
}
