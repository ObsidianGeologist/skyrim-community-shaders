#include "Brixelizer.h"

#include <DDSTextureLoader.h>
#include <DirectXMesh.h>

#include "Deferred.h"
#include "Util.h"

#include "Brixelizer/BrixelizerContext.h"
#include "Brixelizer/BrixelizerGIContext.h"

void Brixelizer::CacheFramebuffer()
{
	auto frameBuffer = (FrameBuffer*)mappedFrameBuffer->pData;
	frameBufferCached = *frameBuffer;
	mappedFrameBuffer = nullptr;
}

void Brixelizer::DrawSettings()
{
	ImGui::Text("Debug capture requires that PIX is attached.");
	if (ImGui::Button("Take Debug Capture") && !debugCapture) {
		debugCapture = true;
	}

	ImGui::Checkbox("update", &update);

	//ImGui::SliderInt("Debug Mode", (int*)&m_DebugMode, 0, FFX_BRIXELIZER_TRACE_DEBUG_MODE_CASCADE_ID, "%d", ImGuiSliderFlags_AlwaysClamp);
	//if (auto _tt = Util::HoverTooltipWrapper())
	//	ImGui::Text("%s", magic_enum::enum_name(m_DebugMode).data());

	//ImGui::SliderInt("Start Cascade", &m_StartCascadeIdx, 0, NUM_BRIXELIZER_CASCADES - 1);
	//ImGui::SliderInt("End Cascade", &m_EndCascadeIdx, 0, NUM_BRIXELIZER_CASCADES - 1);

	//ImGui::SliderFloat("SDF Solve Epsilon", &m_SdfSolveEps, 1e-6f, 1.0f);

	//ImGui::Checkbox("Show Static Instance AABBs", &m_ShowStaticInstanceAABBs);
	//ImGui::Checkbox("Show Dynamic Instance AABBs", &m_ShowDynamicInstanceAABBs);
	//ImGui::Checkbox("Show Cascade AABBs", &m_ShowCascadeAABBs);
	//ImGui::SliderInt("Show AABB Tree Index", &m_ShowAABBTreeIndex, -1, NUM_BRIXELIZER_CASCADES - 1);

	//static FfxBrixelizerStats statsFirstCascade{};

	//if (stats.cascadeIndex == 0) {
	//	statsFirstCascade = stats;
	//}

	//ImGui::NewLine();
	//ImGui::Text("Stats:");
	//ImGui::Text(std::format("	cascadeIndex : {}", statsFirstCascade.cascadeIndex).c_str());
	//ImGui::Text("	staticCascadeStats:");
	//ImGui::Text(std::format("	bricksAllocated : {}", statsFirstCascade.staticCascadeStats.bricksAllocated).c_str());
	//ImGui::Text(std::format("	referencesAllocated : {}", statsFirstCascade.staticCascadeStats.referencesAllocated).c_str());
	//ImGui::Text(std::format("	trianglesAllocated : {}", statsFirstCascade.staticCascadeStats.trianglesAllocated).c_str());

	//ImGui::Text("	dynamicCascadeStats:");
	//ImGui::Text(std::format("	bricksAllocated : {}", statsFirstCascade.dynamicCascadeStats.bricksAllocated).c_str());
	//ImGui::Text(std::format("	referencesAllocated : {}", statsFirstCascade.dynamicCascadeStats.referencesAllocated).c_str());
	//ImGui::Text(std::format("	trianglesAllocated : {}", statsFirstCascade.dynamicCascadeStats.trianglesAllocated).c_str());

	//ImGui::Text("	contextStats:");
	//ImGui::Text(std::format("	brickAllocationsAttempted : {}", statsFirstCascade.contextStats.brickAllocationsAttempted).c_str());
	//ImGui::Text(std::format("	brickAllocationsSucceeded : {}", statsFirstCascade.contextStats.brickAllocationsSucceeded).c_str());
	//ImGui::Text(std::format("	bricksCleared : {}", statsFirstCascade.contextStats.bricksCleared).c_str());
	//ImGui::Text(std::format("	bricksMerged : {}", statsFirstCascade.contextStats.bricksMerged).c_str());
	//ImGui::Text(std::format("	freeBricks : {}", statsFirstCascade.contextStats.freeBricks).c_str());
}

void Brixelizer::InitD3D12(IDXGIAdapter* a_adapter)
{
	DX::ThrowIfFailed(D3D12CreateDevice(a_adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&d3d12Device)));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	DX::ThrowIfFailed(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	DX::ThrowIfFailed(d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

	DX::ThrowIfFailed(d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr, IID_PPV_ARGS(&commandList)));

	debugAvailable = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&ga)) == S_OK;
}

void Brixelizer::InitBrixelizer()
{
	auto manager = RE::BSGraphics::Renderer::GetSingleton();

	auto device = reinterpret_cast<ID3D11Device*>(manager->GetRuntimeData().forwarder);
	auto context = reinterpret_cast<ID3D11DeviceContext*>(manager->GetRuntimeData().context);

	DX::ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&d3d11Device)));
	DX::ThrowIfFailed(context->QueryInterface(IID_PPV_ARGS(&d3d11Context)));

	stl::detour_vfunc<14, Brixelizer::Hooks::ID3D11DeviceContext_Map>(context);
	stl::detour_vfunc<15, Brixelizer::Hooks::ID3D11DeviceContext_Unmap>(context);

	BrixelizerGIContext::GetSingleton()->CreateNoiseTextures();

	InitializeSharedFence();
	OpenSharedHandles();
	BrixelizerContext::GetSingleton()->InitBrixelizerContext();
	BrixelizerGIContext::GetSingleton()->InitBrixelizerGIContext();
}

void Brixelizer::CreatedWrappedResource(D3D11_TEXTURE2D_DESC a_texDesc, Brixelizer::WrappedResource& a_resource)
{
	auto& d3d11Device = GetSingleton()->d3d11Device;

	DX::ThrowIfFailed(d3d11Device->CreateTexture2D(&a_texDesc, nullptr, &a_resource.resource11));

	IDXGIResource1* dxgiResource = nullptr;
	DX::ThrowIfFailed(a_resource.resource11->QueryInterface(IID_PPV_ARGS(&dxgiResource)));

	HANDLE sharedHandle = nullptr;
	DX::ThrowIfFailed(dxgiResource->CreateSharedHandle(
		nullptr,
		DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
		nullptr,
		&sharedHandle));

	DX::ThrowIfFailed(GetSingleton()->d3d12Device->OpenSharedHandle(
		sharedHandle,
		IID_PPV_ARGS(&a_resource.resource)));

	//CloseHandle(sharedHandle);

	if (a_texDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = a_texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		DX::ThrowIfFailed(d3d11Device->CreateShaderResourceView(a_resource.resource11, &srvDesc, &a_resource.srv));
	}

	if (a_texDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = a_texDesc.Format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		DX::ThrowIfFailed(d3d11Device->CreateUnorderedAccessView(a_resource.resource11, &uavDesc, &a_resource.uav));
	}

	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers{
			CD3DX12_RESOURCE_BARRIER::Transition(a_resource.resource.get(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		};
		GetSingleton()->commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}
}

// Function to check for shared NT handle support and convert to D3D12 resource
Brixelizer::RenderTargetDataD3D12 Brixelizer::ConvertD3D11TextureToD3D12(RE::BSGraphics::RenderTargetData* rtData)
{
	RenderTargetDataD3D12 renderTargetData{};

	if (!rtData->texture)
		return renderTargetData;

	D3D11_TEXTURE2D_DESC texDesc{};
	rtData->texture->GetDesc(&texDesc);

	if (!(texDesc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE))
		return renderTargetData;

	// Query the DXGIResource1 interface to access shared NT handle
	winrt::com_ptr<IDXGIResource1> dxgiResource1;
	DX::ThrowIfFailed(rtData->texture->QueryInterface(IID_PPV_ARGS(&dxgiResource1)));

	// Create the shared NT handle
	HANDLE sharedNtHandle = nullptr;
	DX::ThrowIfFailed(dxgiResource1->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &sharedNtHandle));

	// Open the shared handle in D3D12
	DX::ThrowIfFailed(d3d12Device->OpenSharedHandle(sharedNtHandle, IID_PPV_ARGS(&renderTargetData.d3d12Resource)));
	//CloseHandle(sharedNtHandle);  // Close the handle after opening it in D3D12

	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers{
			CD3DX12_RESOURCE_BARRIER::Transition(renderTargetData.d3d12Resource.get(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		};
		GetSingleton()->commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}

	return renderTargetData;
}

void Brixelizer::OpenSharedHandles()
{
	auto renderer = RE::BSGraphics::Renderer::GetSingleton();

	for (int i = 0; i < RE::RENDER_TARGET::kTOTAL; i++) {
		renderTargetsD3D12[i] = ConvertD3D11TextureToD3D12(&renderer->GetRuntimeData().renderTargets[i]);
	}
}

void Brixelizer::ClearShaderCache()
{
}

void Brixelizer::InitializeSharedFence()
{
	DX::ThrowIfFailed(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&d3d12Fence)));

	HANDLE sharedFenceHandle;

	DX::ThrowIfFailed(d3d12Device->CreateSharedHandle(d3d12Fence.get(), nullptr, GENERIC_ALL, nullptr, &sharedFenceHandle));
	DX::ThrowIfFailed(d3d11Device->OpenSharedFence(sharedFenceHandle, IID_PPV_ARGS(&d3d11Fence)));
}

void Brixelizer::FrameUpdate()
{
	BrixelizerGIContext::GetSingleton()->CopyResourcesToSharedBuffers();

	// Wait for D3D11 to complete prior work
	DX::ThrowIfFailed(d3d11Context->Signal(d3d11Fence.get(), ++currentFenceValue));
	DX::ThrowIfFailed(commandQueue->Wait(d3d12Fence.get(), currentFenceValue));

	if (debugAvailable && debugCapture)
		ga->BeginCapture();

	BrixelizerContext::GetSingleton()->UpdateBrixelizerContext();
	BrixelizerGIContext::GetSingleton()->UpdateBrixelizerGIContext();

	DX::ThrowIfFailed(commandList->Close());

	ID3D12CommandList* ppCommandLists[] = { commandList.get() };
	commandQueue->ExecuteCommandLists(1, ppCommandLists);

	DX::ThrowIfFailed(commandAllocator->Reset());
	DX::ThrowIfFailed(commandList->Reset(commandAllocator.get(), nullptr));

	if (debugAvailable && debugCapture)
		ga->EndCapture();

	// Signal and wait for D3D12 to complete this frame's work
	DX::ThrowIfFailed(commandQueue->Signal(d3d12Fence.get(), ++currentFenceValue));
	DX::ThrowIfFailed(d3d11Context->Wait(d3d11Fence.get(), currentFenceValue));

	debugCapture = false;
}