// Shader de los billboards: quads con una imagen que siempre miran a la
// camara. Las matrices se pasan en fila (row_major) y se multiplican con
// el vector a la izquierda, igual que en DepuradorColision y PanelRR.

Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);

cbuffer Matrices : register(b0)
{
	row_major float4x4 mundo;
	row_major float4x4 vistaProyeccion;
};

struct VS_Input
{
	float3 pos : POSITION;
	float2 tex0 : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float2 tex0 : TEXCOORD0;
};

PS_Input VS_Main(VS_Input vertex)
{
	PS_Input salida;

	// El vertice solo trae XYZ: la W debe ser 1 para que la traslacion
	// de la matriz de mundo se aplique.
	salida.pos = mul(mul(float4(vertex.pos, 1.0f), mundo), vistaProyeccion);
	salida.tex0 = vertex.tex0;

	return salida;
}

float4 PS_Main(PS_Input pix) : SV_TARGET
{
	float4 color = colorMap.Sample(colorSampler, pix.tex0);

	// Los pixeles transparentes no se dibujan y tampoco escriben depth.
	clip(color.a - 0.01f);

	return color;
}
