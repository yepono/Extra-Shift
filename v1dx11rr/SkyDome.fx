Texture2D textures : register(t0);
SamplerState colorSampler : register(s0);

cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projMatrix;
	// x: 1 = textura HDR equirectangular, 0 = textura LDR con UV del domo
	// y: exposicion (solo HDR)
	// z: altura de la camara sobre el centro del domo
	float4 valores;
};

static const float PI = 3.14159265f;

struct VS_Input
{
	float4 pos : POSITION;
	float2 tex0 : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float2 tex0 : TEXCOORD0;
	float3 dir : TEXCOORD1;
};

PS_Input VS_Main(VS_Input vertex)
{
	PS_Input vsOut = (PS_Input)0;
	vsOut.pos = mul(vertex.pos, worldMatrix);
	vsOut.pos = mul(vsOut.pos, viewMatrix);
	vsOut.pos = mul(vsOut.pos, projMatrix);

	vsOut.tex0 = vertex.tex0;
	// Direccion desde la camara (no desde el centro del domo, que esta
	// valores.z unidades mas abajo); si no, el cielo se ve "con zoom".
	vsOut.dir = vertex.pos.xyz - float3(0.0f, valores.z, 0.0f);

	return vsOut;
}

// Aproximacion ACES (Narkowicz) para llevar HDR lineal a [0,1]
float3 ToneMapACES(float3 x)
{
	return saturate((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f));
}

float4 PS_Main(PS_Input pix) : SV_TARGET
{
	if (valores.x < 0.5f)
		return textures.Sample(colorSampler, pix.tex0);

	// Mapeo equirectangular: u = longitud, v = colatitud
	float3 d = normalize(pix.dir);
	float2 uv;
	uv.x = atan2(d.x, d.z) / (2.0f * PI) + 0.5f;
	uv.y = acos(clamp(d.y, -1.0f, 1.0f)) / PI;

	// SampleLevel evita la costura en el salto de atan2 (no hay mipmaps)
	float3 hdr = textures.SampleLevel(colorSampler, uv, 0).rgb * valores.y;
	float3 color = pow(ToneMapACES(hdr), 1.0f / 2.2f);

	return float4(color, 1.0f);
}
