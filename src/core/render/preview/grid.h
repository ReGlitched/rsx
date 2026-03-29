#pragma once

#include <core/render/dxshader.h>

constexpr static const char s_GridPixelShader[]{
	"struct VS_Output\n"
	"{\n"
	"    float4 position : SV_POSITION;\n"
	"    float4 color : COLOR;\n"
	"};\n"
	"float4 ps_main(VS_Output input) : SV_Target\n"
	"{\n"
	"    return input.color;\n"
	"}"
};

constexpr static const char s_GridVertexShader[]{
	"struct VS_Input\n"
	"{\n"
	"    float3 position : POSITION;\n"
	"    uint normal : NORMAL;\n"
	"    uint color : COLOR;\n"
	"    float2 uv : TEXCOORD;\n"
	"    uint weight : BLENDWEIGHT;\n"
	"};\n"
	"struct VS_Output\n"
	"{\n"
	"    float4 position : SV_POSITION;\n"
	"    float4 color : COLOR;\n"
	"};\n"
	"cbuffer VS_TransformConstants : register(b0)\n"
	"{\n"
	"    float4x4 modelMatrix;\n"
	"    float4x4 viewMatrix;\n"
	"    float4x4 projectionMatrix;\n"
	"};\n"
	"VS_Output vs_main(VS_Input input)\n"
	"{\n"
	"    VS_Output output;\n"
	"    float3 pos = float3(-input.position.x, input.position.y, input.position.z);\n"
	"    output.position = mul(float4(pos, 1.f), modelMatrix);\n"
	"    output.position = mul(output.position, viewMatrix);\n"
	"    output.position = mul(output.position, projectionMatrix);\n"
	"    output.color = float4(((input.normal >> 24) & 0xFF) / 255.f, ((input.normal >> 16) & 0xFF) / 255.f, ((input.normal >> 8) & 0xFF) / 255.f, ((input.normal) & 0xFF) / 255.f);\n"
	"    return output;\n"
	"}"
};

struct GridVertex_t
{
	float x, y, z;
	uint32_t _normal_unused;
	uint32_t color;
	float tex_u, tex_v; // unused

	constexpr GridVertex_t() {};
	constexpr GridVertex_t(float x, float y, float z, uint32_t color) : x(x), y(y), z(z), _normal_unused(color), color(color), tex_u(0.f), tex_v(0.f) {};
};
class CShader;

template<int N, int SZ=1>
struct PreviewGrid_t
{
	constexpr PreviewGrid_t() : numVertices(4*(N+1))
	{
		constexpr float minCoord = (-(N / 2)) * SZ;
		constexpr float maxCoord = (N / 2) * SZ;
		constexpr uint16_t horizontalVertsStart = (2 * (N + 1));

		uint16_t numVertsWritten = 0;
		for (float i = minCoord; i <= maxCoord; i += SZ)
		{
			constexpr uint32_t lineColour = 0x919191FF;
			vertices[numVertsWritten] = { i, 0, maxCoord, lineColour };
			vertices[numVertsWritten+1] = { i, 0, minCoord, lineColour };

			vertices[numVertsWritten + horizontalVertsStart] = { minCoord, 0, i, lineColour };
			vertices[numVertsWritten + horizontalVertsStart + 1] = { maxCoord, 0, i, lineColour };

			numVertsWritten += 2;
		}
	};

	GridVertex_t vertices[4*(N+1)];

	UINT numVertices;
	UINT vertexStride;

	ID3D11Buffer* vertexBuffer;

	CShader* vertexShader;
	CShader* pixelShader;

	void CreateBuffers(ID3D11Device* device, CDXShaderManager* shaderManager)
	{
		if (!vertexBuffer)
		{
			constexpr UINT vertStride = sizeof(GridVertex_t);

			D3D11_BUFFER_DESC desc = {};

			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.ByteWidth = static_cast<UINT>(vertStride * numVertices);
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA srd{ vertices };

			if (FAILED(device->CreateBuffer(&desc, &srd, &vertexBuffer)))
				return;

			vertexStride = vertStride;

			vertexShader = shaderManager->LoadShaderFromString("preview/grid_vs", s_GridVertexShader, eShaderType::Vertex);
			pixelShader = shaderManager->LoadShaderFromString("preview/grid_ps", s_GridPixelShader, eShaderType::Pixel);

		}
	}

	void Draw(ID3D11DeviceContext* ctx)
	{
		UINT offset = 0u;

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		ctx->IASetVertexBuffers(0u, 1u, &vertexBuffer, &vertexStride, &offset);

		ctx->IASetInputLayout(vertexShader->GetInputLayout());
		ctx->VSSetShader(vertexShader->Get<ID3D11VertexShader>(), nullptr, 0u);
		ctx->PSSetShader(pixelShader->Get<ID3D11PixelShader>(), nullptr, 0u);

		ctx->Draw(numVertices, 0);
	}
};