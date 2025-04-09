#include "ShellTexturing.h"

#include <DDSTextureLoader.h>
#include <DirectXTex.h>

#include "Deferred.h"
#include "Globals.h"
#include "ShaderCache.h"
#include "State.h"

constexpr auto MIPLEVELS = 8;

//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShellTexturing::Settings);

void ShellTexturing::DrawSettings()
{
	if (ImGui::TreeNodeEx("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
	}
	ImGui::TreePop();
}

void ShellTexturing::PostPostLoad()
{
	Hooks::Install();
}

//void ShellTexturing::LoadSettings(json& o_json)
//{
//	settings = o_json;
//}
//
//void ShellTexturing::SaveSettings(json& o_json)
//{
//	o_json = settings;
//}

void ShellTexturing::RestoreDefaultSettings()
{
	settings = {};
}

void ShellTexturing::ClearShaderCache()
{
}

ID3D11ComputeShader* ShellTexturing::ApplyShellTexture()
{
	logger::debug("Compiling ApplyShellTexture");
	applyShellTexture = static_cast<ID3D11ComputeShader*>(Util::CompileShader(L"Data\\Shaders\\ShellTexturing\\ApplyShellTexture.hlsl", {}, "cs_5_0"));

	return applyShellTexture;
}

void ShellTexturing::BSLightingShader_SetupShells(RE::BSRenderPass* a_pass)
{
	auto state = globals::state;

		if (a_pass->shaderProperty->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kModelSpaceNormals, RE::BSShaderProperty::EShaderPropertyFlag::kSkinned)) {
			bool hasShells = true;

			auto geometry = a_pass->geometry;
			if (auto userData = geometry->GetUserData()) {
				auto actor = userData->As<RE::Actor>();
				if (actor != nullptr) {
					auto race = actor->GetRace();
					if (race != nullptr) {
						hasShells = race->HasKeywordByEditorID("ActorTypeAnimal");
					}
				}
			}
			
			if (hasShells)
				state->currentExtraDescriptor |= (uint)State::ExtraShaderDescriptors::HasShells;
		}
}

ID3D11GeometryShader* ShellTexturing::GenerateLayers()
{
	logger::debug("Compiling GenerateShells");
	generateShells = static_cast<ID3D11GeometryShader*>(Util::CompileShader(L"Data\\Shaders\\ShellTexturing\\GenerateShells.hlsl", {}, "cs_5_0"));
	return generateShells;
}

void ShellTexturing::SetupResources()
{
	//auto renderer = globals::game::renderer;
	auto device = globals::d3d::device;

	DirectX::CreateDDSTextureFromFile(device, L"Data\\textures\\ShellTexturing", nullptr, &defaultNoiseTexture);
}

void ShellTexturing::Reset()
{
}

void ShellTexturing::Hooks::BSLightingShader_SetupGeometry::thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags)
{
	globals::features::shellTexturing->BSLightingShader_SetupShells(Pass);
	func(This, Pass, RenderFlags);
}
