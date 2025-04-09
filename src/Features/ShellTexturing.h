#pragma once


struct ShellTexturing : Feature
{
public:
	static ShellTexturing* GetSingleton()
	{
		static ShellTexturing singleton;
		return &singleton;
	}

	ID3D11ShaderResourceView* defaultNoiseTexture = nullptr;
	ID3D11ComputeShader* applyShellTexture = nullptr;
	ID3D11GeometryShader* generateShells = nullptr;

	struct Settings
	{
		std::vector<std::string> meshes;
	};

	Settings settings;
	
	virtual inline std::string GetName() override { return "Shell Texturing"; }
	virtual inline std::string GetShortName() override { return "ShellTexturing"; }
	virtual inline std::string_view GetShaderDefineName() override { return "SHELL_TEXTURING"; }

	virtual void SetupResources() override;
	virtual void Reset() override;

	/*virtual void SaveSettings(json&) override;
	virtual void LoadSettings(json&) override;*/
	virtual void RestoreDefaultSettings() override;
	virtual void DrawSettings() override;
	virtual void PostPostLoad() override;

	virtual void ClearShaderCache() override;

	ID3D11GeometryShader* GenerateLayers();
	ID3D11ComputeShader* ApplyShellTexture();

	void BSLightingShader_SetupShells(RE::BSRenderPass* Pass);

	virtual bool SupportsVR() override { return true; };
	virtual bool IsCore() const override { return false; };

	struct Hooks
	{
		struct BSLightingShader_SetupGeometry
		{
			static void thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags);
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			stl::write_vfunc<0x6, BSLightingShader_SetupGeometry>(RE::VTABLE_BSLightingShader[0]);
			logger::info("[SHELL TEXTURING] Installed hooks");
		}
	};
};
