#include "Weather.h"

#include "Deferred.h"
#include "Util.h"

void Weather::Bind()
{
	if (loaded) {
		auto& context = State::GetSingleton()->context;

		// Set PS shader resource
		{
			ID3D11ShaderResourceView* srv = diffuseIBLTexture->srv.get();
			context->PSSetShaderResources(80, 1, &srv);
		}
	}
}

void Weather::Prepass()
{
	auto& context = State::GetSingleton()->context;

	auto renderer = RE::BSGraphics::Renderer::GetSingleton();
	auto& reflections = renderer->GetRendererData().cubemapRenderTargets[RE::RENDER_TARGET_CUBEMAP::kREFLECTIONS];

	std::array<ID3D11ShaderResourceView*, 1> srvs = { reflections.SRV };
	std::array<ID3D11UnorderedAccessView*, 1> uavs = { diffuseIBLTexture->uav.get() };
	std::array<ID3D11SamplerState*, 1> samplers = { Deferred::GetSingleton()->linearSampler };

	// Unset PS shader resource
	{
		ID3D11ShaderResourceView* srv = nullptr;
		context->PSSetShaderResources(80, 1, &srv);
	}

	// IBL
	{
		samplers[0] = Deferred::GetSingleton()->linearSampler;

		context->CSSetSamplers(0, (uint)samplers.size(), samplers.data());
		context->CSSetShaderResources(0, (uint)srvs.size(), srvs.data());
		context->CSSetUnorderedAccessViews(0, (uint)uavs.size(), uavs.data(), nullptr);
		context->CSSetShader(GetDiffuseIBLCS(), nullptr, 0);
		context->Dispatch(1, 1, 1);
	}

	// Reset
	{
		srvs.fill(nullptr);
		uavs.fill(nullptr);
		samplers.fill(nullptr);

		context->CSSetSamplers(0, (uint)samplers.size(), samplers.data());
		context->CSSetShaderResources(0, (uint)srvs.size(), srvs.data());
		context->CSSetUnorderedAccessViews(0, (uint)uavs.size(), uavs.data(), nullptr);
		context->CSSetShader(nullptr, nullptr, 0);
	}

	// Set PS shader resource
	{
		ID3D11ShaderResourceView* srv = diffuseIBLTexture->srv.get();
		context->PSSetShaderResources(80, 1, &srv);
	}
}

void Weather::SetupResources()
{
	GetDiffuseIBLCS();

	{
		D3D11_TEXTURE2D_DESC texDesc{
			.Width = 3,
			.Height = 1,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.SampleDesc = { 1, 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = texDesc.Format,
			.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = texDesc.MipLevels }
		};
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = texDesc.Format,
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D,
			.Texture2D = { .MipSlice = 0 }
		};

		diffuseIBLTexture = new Texture2D(texDesc);
		diffuseIBLTexture->CreateSRV(srvDesc);
		diffuseIBLTexture->CreateUAV(uavDesc);
	}
}

void Weather::ClearShaderCache()
{
	if (diffuseIBLCS)
		diffuseIBLCS->Release();
	diffuseIBLCS = nullptr;
}

ID3D11ComputeShader* Weather::GetDiffuseIBLCS()
{
	if (!diffuseIBLCS)
		diffuseIBLCS = static_cast<ID3D11ComputeShader*>(Util::CompileShader(L"Data\\Shaders\\Weather\\DiffuseIBLCS.hlsl", {}, "cs_5_0"));
	return diffuseIBLCS;
}
