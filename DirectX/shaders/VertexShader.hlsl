cbuffer CBuf
{
    matrix transform;
};

struct Input {
   // float3 color : COLOR; //no longer needed because of constantbuffer2 with colors
	float3 position : POSITION;
    float2 texCoord : TEXTCOORD;
};


struct Output {
  //  float4 color : COLOR;
	float4 position : SV_POSITION;
    float2 texCoord : TEXTCOORD;
};


Output main(Input input){
	
	Output output;

    output.position = mul(float4(input.position, 1.0f), transform);
    output.texCoord = input.texCoord;
   // output.color = (input.color.r, input.color.g, input.color.b, 1.0f);
        
    return output;
}