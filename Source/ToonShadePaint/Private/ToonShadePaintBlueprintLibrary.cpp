// Copyright © 2024-2025 kafues511 All Rights Reserved.

#include "ToonShadePaintBlueprintLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Algo/Count.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ToonShadeCaptureTargetActor.h"


DEFINE_LOG_CATEGORY(LogToonShadePaint);


class FSetupSeedFlagsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSetupSeedFlagsCS, Global);

public:
	FSetupSeedFlagsCS() = default;
	explicit FSetupSeedFlagsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		SeedTexture.Bind(Initializer.ParameterMap, TEXT("SeedTexture"));
		RWSeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("RWSeedFlagsTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		uint32 InLayerIndex,
		FRHITexture* InSeedTexture,
		FRHIUnorderedAccessView* InRWSeedFlagsTexture)
	{
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetTextureParameter(BatchedParameters, SeedTexture, InSeedTexture);
		SetUAVParameter(BatchedParameters, RWSeedFlagsTexture, InRWSeedFlagsTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetUAVParameter(BatchedUnbinds, RWSeedFlagsTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderResourceParameter, SeedTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSeedFlagsTexture);
};

class FSetupPosCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSetupPosCS, Global);

public:
	FSetupPosCS() = default;
	explicit FSetupPosCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		RWPositionTexture.Bind(Initializer.ParameterMap, TEXT("RWPositionTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		FRHITexture* InPositionTexture,
		FRHIUnorderedAccessView* InRWPositionTexture)
	{
		SetTextureParameter(BatchedParameters, PositionTexture, InPositionTexture);
		SetUAVParameter(BatchedParameters, RWPositionTexture, InRWPositionTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetUAVParameter(BatchedUnbinds, RWPositionTexture);
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, PositionTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWPositionTexture);
};

class FDistanceMapSetupCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceMapSetupCS, Global);

public:
	FDistanceMapSetupCS() = default;
	explicit FDistanceMapSetupCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		SeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("SeedFlagsTexture"));
		RWSDFInnerTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFInnerTexture"));
		RWSDFOuterTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFOuterTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		uint32 InLayerIndex,
		FRHIShaderResourceView* InSeedFlagsTexture,
		FRHIUnorderedAccessView* InRWSDFInnerTexture,
		FRHIUnorderedAccessView* InRWSDFOuterTexture)
	{
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetSRVParameter(BatchedParameters, SeedFlagsTexture, InSeedFlagsTexture);
		SetUAVParameter(BatchedParameters, RWSDFInnerTexture, InRWSDFInnerTexture);
		SetUAVParameter(BatchedParameters, RWSDFOuterTexture, InRWSDFOuterTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, SeedFlagsTexture);
		UnsetUAVParameter(BatchedUnbinds, RWSDFInnerTexture);
		UnsetUAVParameter(BatchedUnbinds, RWSDFOuterTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderResourceParameter, SeedFlagsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFInnerTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFOuterTexture);
};

class FDistanceMapIterCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceMapIterCS, Global);

	class FFlip : SHADER_PERMUTATION_BOOL("FLIP");

	using FPermutationDomain = TShaderPermutationDomain<FFlip>;

public:
	FDistanceMapIterCS() = default;
	explicit FDistanceMapIterCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		TextureSize.Bind(Initializer.ParameterMap, TEXT("TextureSize"));
		Radius.Bind(Initializer.ParameterMap, TEXT("Radius"));
		SeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("SeedFlagsTexture"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		RWSDFInnerTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFInnerTexture"));
		RWSDFOuterTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFOuterTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		uint32 InLayerIndex,
		FIntPoint InTextureSize,
		int32 InRadius,
		FRHIShaderResourceView* InSeedFlagsTexture,
		FRHIShaderResourceView* InPositionTexture,
		FRHIUnorderedAccessView* InRWSDFInnerTexture,
		FRHIUnorderedAccessView* InRWSDFOuterTexture)
	{
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetShaderValue(BatchedParameters, TextureSize, InTextureSize);
		SetShaderValue(BatchedParameters, Radius, InRadius);
		SetSRVParameter(BatchedParameters, SeedFlagsTexture, InSeedFlagsTexture);
		SetSRVParameter(BatchedParameters, PositionTexture, InPositionTexture);
		SetUAVParameter(BatchedParameters, RWSDFInnerTexture, InRWSDFInnerTexture);
		SetUAVParameter(BatchedParameters, RWSDFOuterTexture, InRWSDFOuterTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, SeedFlagsTexture);
		UnsetSRVParameter(BatchedUnbinds, PositionTexture);
		UnsetUAVParameter(BatchedUnbinds, RWSDFInnerTexture);
		UnsetUAVParameter(BatchedUnbinds, RWSDFOuterTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderParameter, TextureSize);
	LAYOUT_FIELD(FShaderParameter, Radius);
	LAYOUT_FIELD(FShaderResourceParameter, SeedFlagsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, PositionTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFInnerTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFOuterTexture);
};

class FSDFCalcCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSDFCalcCS, Global);

	class FFlip : SHADER_PERMUTATION_BOOL("FLIP");

	using FPermutationDomain = TShaderPermutationDomain<FFlip>;

public:
	FSDFCalcCS() = default;
	explicit FSDFCalcCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		TextureSize.Bind(Initializer.ParameterMap, TEXT("TextureSize"));
		SeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("SeedFlagsTexture"));
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		SDFInnerTexture.Bind(Initializer.ParameterMap, TEXT("SDFInnerTexture"));
		SDFOuterTexture.Bind(Initializer.ParameterMap, TEXT("SDFOuterTexture"));
		RWSDFTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFTexture"));
		RWMaxDistanceBuffer.Bind(Initializer.ParameterMap, TEXT("RWMaxDistanceBuffer"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		uint32 InLayerIndex,
		FIntPoint InTextureSize,
		FRHIShaderResourceView* InSeedFlagsTexture,
		FRHIShaderResourceView* InPositionTexture,
		FRHIShaderResourceView* InSDFInnerTexture,
		FRHIShaderResourceView* InSDFOuterTexture,
		FRHIUnorderedAccessView* InRWSDFTexture,
		FRHIUnorderedAccessView* InRWMaxDistanceBuffer)
	{
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetShaderValue(BatchedParameters, TextureSize, InTextureSize);
		SetSRVParameter(BatchedParameters, SeedFlagsTexture, InSeedFlagsTexture);
		SetSRVParameter(BatchedParameters, PositionTexture, InPositionTexture);
		SetSRVParameter(BatchedParameters, SDFInnerTexture, InSDFInnerTexture);
		SetSRVParameter(BatchedParameters, SDFOuterTexture, InSDFOuterTexture);
		SetUAVParameter(BatchedParameters, RWSDFTexture, InRWSDFTexture);
		SetUAVParameter(BatchedParameters, RWMaxDistanceBuffer, InRWMaxDistanceBuffer);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, SeedFlagsTexture);
		UnsetSRVParameter(BatchedUnbinds, PositionTexture);
		UnsetSRVParameter(BatchedUnbinds, SDFInnerTexture);
		UnsetSRVParameter(BatchedUnbinds, SDFOuterTexture);
		UnsetUAVParameter(BatchedUnbinds, RWSDFTexture);
		UnsetUAVParameter(BatchedUnbinds, RWMaxDistanceBuffer);
	}

private:
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderParameter, TextureSize);
	LAYOUT_FIELD(FShaderResourceParameter, SeedFlagsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, PositionTexture);
	LAYOUT_FIELD(FShaderResourceParameter, SDFInnerTexture);
	LAYOUT_FIELD(FShaderResourceParameter, SDFOuterTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWMaxDistanceBuffer);
};

class FSDFNormalizedCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSDFNormalizedCS, Global);

public:
	FSDFNormalizedCS() = default;
	explicit FSDFNormalizedCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		MaxDistanceBuffer.Bind(Initializer.ParameterMap, TEXT("MaxDistanceBuffer"));
		RWSDFNormalizedTexture.Bind(Initializer.ParameterMap, TEXT("RWSDFNormalizedTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		uint32 InLayerIndex,
		FRHIShaderResourceView* InMaxDistanceBuffer,
		FRHIUnorderedAccessView* InRWSDFNormalizedTexture)
	{
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetSRVParameter(BatchedParameters, MaxDistanceBuffer, InMaxDistanceBuffer);
		SetUAVParameter(BatchedParameters, RWSDFNormalizedTexture, InRWSDFNormalizedTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, MaxDistanceBuffer);
		UnsetUAVParameter(BatchedUnbinds, RWSDFNormalizedTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderResourceParameter, MaxDistanceBuffer);
	LAYOUT_FIELD(FShaderResourceParameter, RWSDFNormalizedTexture);
};

class FSDFBlendCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSDFBlendCS, Global);

	class FFlip : SHADER_PERMUTATION_BOOL("FLIP");

	using FPermutationDomain = TShaderPermutationDomain<FFlip>;

public:
	FSDFBlendCS() = default;
	explicit FSDFBlendCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		Start.Bind(Initializer.ParameterMap, TEXT("Start"));
		End.Bind(Initializer.ParameterMap, TEXT("End"));
		LayerIndex.Bind(Initializer.ParameterMap, TEXT("LayerIndex"));
		SeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("SeedFlagsTexture"));
		SDFNormalizedTexture.Bind(Initializer.ParameterMap, TEXT("SDFNormalizedTexture"));
		RWShadowThresholdTexture.Bind(Initializer.ParameterMap, TEXT("RWShadowThresholdTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		float InStart,
		float InEnd,
		uint32 InLayerIndex,
		FRHIShaderResourceView* InSeedFlagsTexture,
		FRHIShaderResourceView* InSDFNormalizedTexture,
		FRHIUnorderedAccessView* InRWShadowThresholdTexture)
	{
		SetShaderValue(BatchedParameters, Start, InStart);
		SetShaderValue(BatchedParameters, End, InEnd);
		SetShaderValue(BatchedParameters, LayerIndex, InLayerIndex);
		SetSRVParameter(BatchedParameters, SeedFlagsTexture, InSeedFlagsTexture);
		SetSRVParameter(BatchedParameters, SDFNormalizedTexture, InSDFNormalizedTexture);
		SetUAVParameter(BatchedParameters, RWShadowThresholdTexture, InRWShadowThresholdTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, SeedFlagsTexture);
		UnsetSRVParameter(BatchedUnbinds, SDFNormalizedTexture);
		UnsetUAVParameter(BatchedUnbinds, RWShadowThresholdTexture);
	}

private:
	LAYOUT_FIELD(FShaderParameter, Start);
	LAYOUT_FIELD(FShaderParameter, End);
	LAYOUT_FIELD(FShaderParameter, LayerIndex);
	LAYOUT_FIELD(FShaderResourceParameter, SeedFlagsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, SDFNormalizedTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWShadowThresholdTexture);
};

class FShadowThresholdCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowThresholdCS, Global);

	class FFlip : SHADER_PERMUTATION_BOOL("FLIP");

	using FPermutationDomain = TShaderPermutationDomain<FFlip>;

public:
	FShadowThresholdCS() = default;
	explicit FShadowThresholdCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		SeedFlagsTexture.Bind(Initializer.ParameterMap, TEXT("SeedFlagsTexture"));
		ShadowThresholdTexture.Bind(Initializer.ParameterMap, TEXT("ShadowThresholdTexture"));
		RWShadowThresholdTexture.Bind(Initializer.ParameterMap, TEXT("RWShadowThresholdTexture"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsPCPlatform(Parameters.Platform) && IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		FRHIShaderResourceView* InSeedFlagsTexture,
		FRHIShaderResourceView* InShadowThresholdTexture,
		FRHIUnorderedAccessView* InRWShadowThresholdTexture)
	{
		SetSRVParameter(BatchedParameters, SeedFlagsTexture, InSeedFlagsTexture);
		SetSRVParameter(BatchedParameters, ShadowThresholdTexture, InShadowThresholdTexture);
		SetUAVParameter(BatchedParameters, RWShadowThresholdTexture, InRWShadowThresholdTexture);
	}

	void UnsetParameters(FRHIBatchedShaderUnbinds& BatchedUnbinds)
	{
		UnsetSRVParameter(BatchedUnbinds, SeedFlagsTexture);
		UnsetSRVParameter(BatchedUnbinds, ShadowThresholdTexture);
		UnsetUAVParameter(BatchedUnbinds, RWShadowThresholdTexture);
	}

private:
	LAYOUT_FIELD(FShaderResourceParameter, SeedFlagsTexture);
	LAYOUT_FIELD(FShaderResourceParameter, ShadowThresholdTexture);
	LAYOUT_FIELD(FShaderResourceParameter, RWShadowThresholdTexture);
};


IMPLEMENT_SHADER_TYPE(, FSetupSeedFlagsCS,		TEXT("/Plugin/ToonShadePaint/Private/SetupSeedFlags.usf"),		TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSetupPosCS,			TEXT("/Plugin/ToonShadePaint/Private/SetupPos.usf"),			TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FDistanceMapSetupCS,	TEXT("/Plugin/ToonShadePaint/Private/DistanceMapSetup.usf"),	TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FDistanceMapIterCS,		TEXT("/Plugin/ToonShadePaint/Private/DistanceMapIter.usf"),		TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSDFCalcCS,				TEXT("/Plugin/ToonShadePaint/Private/SDFCalc.usf"),				TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSDFNormalizedCS,		TEXT("/Plugin/ToonShadePaint/Private/SDFNormalized.usf"),		TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FSDFBlendCS,			TEXT("/Plugin/ToonShadePaint/Private/SDFBlend.usf"),			TEXT("MainCS"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FShadowThresholdCS,		TEXT("/Plugin/ToonShadePaint/Private/ShadowThreshold.usf"),		TEXT("MainCS"), SF_Compute);


static void Initialize2DArray(FRHICommandListBase& RHICmdList, FTextureRWBuffer& SeedFlagsTexture, const TCHAR* InDebugName, uint32 BytesPerElement, uint32 SizeX, uint32 SizeY, int32 ArraySize, EPixelFormat Format, ETextureCreateFlags Flags)
{
	SeedFlagsTexture.NumBytes = SizeX * SizeY * ArraySize * BytesPerElement;

	const FRHITextureCreateDesc Desc =
		FRHITextureCreateDesc::Create2DArray(InDebugName, SizeX, SizeY, ArraySize, Format)
		.SetFlags(Flags);

	SeedFlagsTexture.Buffer = RHICreateTexture(Desc);
	SeedFlagsTexture.UAV = RHICmdList.CreateUnorderedAccessView(SeedFlagsTexture.Buffer);
	SeedFlagsTexture.SRV = RHICmdList.CreateShaderResourceView(SeedFlagsTexture.Buffer, 0);
}


void UToonShadePaintBlueprintLibrary::CreateShadowThresholdMap(
	UObject* WorldContextObject,
	TArray<UTextureRenderTarget2D*> InSeedTextures,
	UTextureRenderTarget2D* InPositionTexture,
	int32 MaxRadius,
	UTextureRenderTarget2D* OutShadowThresholdMapTexture)
{
	const double StartTime = FPlatformTime::Seconds();

	FEvent* Signal = FGenericPlatformProcess::GetSynchEventFromPool(false);

	ENQUEUE_RENDER_COMMAND(ToonShadePaintBlueprintLibrary_CreateShadowThresholdMap)(
		[&InSeedTextures, &InPositionTexture, MaxRadius, &OutShadowThresholdMapTexture, &Signal](FRHICommandListImmediate& RHICmdList)
	{
		RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);

		if (!IsValid(OutShadowThresholdMapTexture))
		{
			UE_LOG(LogToonShadePaint, Warning, TEXT("Invalid 'OutShadowThresholdMapTexture'"));
			Signal->Trigger();
			return;  // 出力先が欲しい
		}

		if (!IsValid(InPositionTexture))
		{
			UE_LOG(LogToonShadePaint, Warning, TEXT("Invalid 'InPositionTexture'"));
			Signal->Trigger();
			return;  // モデル座標が欲しい
		}

		const int32 NumSeedTextures = Algo::CountIf(InSeedTextures, [](const UTextureRenderTarget2D* InSeedTexture) { return IsValid(InSeedTexture); });
		if (NumSeedTextures < 2)
		{
			UE_LOG(LogToonShadePaint, Warning, TEXT("Requires at least two valid 'InSeedTextures'"));
			Signal->Trigger();
			return;  // 最低でも2枚は必要
		}

		const int32 NumGradients = NumSeedTextures - 1;
		const float InvNumGradients = 1.0f / NumGradients;

		// テクスチャ画像は先頭に合わせる
		const int32 Resolution = (*Algo::FindByPredicate(InSeedTextures, [](const UTextureRenderTarget2D* InSeedTexture) { return IsValid(InSeedTexture); }))->SizeX;

		const int32 NumMismatchSeedTextures = Algo::CountIf(InSeedTextures, [Resolution](const UTextureRenderTarget2D* InSeedTexture)
		{
			if (IsValid(InSeedTexture) && InSeedTexture->SizeX != Resolution)
			{
				// RT作る時にName未指定だからログ出しても訳分からないけど、ないよりマシ
				UE_LOG(LogToonShadePaint, Warning, TEXT("Texture size for '%s' is '%d', but requests '%d'"), *InSeedTexture->GetName(), InSeedTexture->SizeX, Resolution);
				return true;
			}
			return false;
		});
		if (NumMismatchSeedTextures > 0)
		{
			Signal->Trigger();
			return;  // SeedTexturesの解像度がバラバラ
		}

		if (InPositionTexture->SizeX != Resolution)
		{
			UE_LOG(LogToonShadePaint, Warning, TEXT("Texture size for '%s' is '%d', but requests '%d'"), *InPositionTexture->GetName(), InPositionTexture->SizeX, Resolution);
			Signal->Trigger();
			return;  // モデル座標の解像度が不一致
		}

		if (OutShadowThresholdMapTexture->SizeX != Resolution)
		{
			UE_LOG(LogToonShadePaint, Warning, TEXT("Texture size for '%s' is '%d', but requests '%d'"), *OutShadowThresholdMapTexture->GetName(), OutShadowThresholdMapTexture->SizeX, Resolution);
			Signal->Trigger();
			return;  // 出力の解像度が不一致
		}

		const EPixelFormat PixelFormat = OutShadowThresholdMapTexture->GetFormat();
		switch (PixelFormat)
		{
		case EPixelFormat::PF_R8G8B8A8:
		case EPixelFormat::PF_FloatRGBA:
		case EPixelFormat::PF_A32B32G32R32F:
			break;
		default:
			UE_LOG(LogToonShadePaint, Warning, TEXT("'%s' only supports PF_R8G8B8A8, PF_FloatRGBA, PF_A32B32G32R32F formats."), *OutShadowThresholdMapTexture->GetName());
			Signal->Trigger();
			return;  // 出力の解像度が不一致
		}

		const FIntPoint TextureSize(Resolution, Resolution);

		const uint32 ThreadGroupCountX = Resolution / 32;
		const uint32 ThreadGroupCountY = Resolution / 32;
		const uint32 ThreadGroupCountZ = 1;

		const ETextureCreateFlags TextureCreateFlags(TexCreate_ShaderResource | TexCreate_UAV);

		FTextureRWBuffer SeedFlagsTexture;
		Initialize2DArray(RHICmdList, SeedFlagsTexture, TEXT("ToonShadePaint.SeedFlagsTexture"), GPixelFormats[PF_R8_UINT].BlockBytes, Resolution, Resolution, NumSeedTextures, PF_R8_UINT, TextureCreateFlags);

		FTextureRWBuffer PositionTexture;
		PositionTexture.Initialize2D(TEXT("ToonShadePaint.PositionTexture"), GPixelFormats[PF_A32B32G32R32F].BlockBytes, Resolution, Resolution, PF_A32B32G32R32F, TextureCreateFlags);

		FTextureRWBuffer SDFInnerTexture;
		SDFInnerTexture.Initialize2D(TEXT("ToonShadePaint.SDFInnerTexture"), GPixelFormats[PF_FloatRGBA].BlockBytes, Resolution, Resolution, PF_FloatRGBA, TextureCreateFlags);

		FTextureRWBuffer SDFOuterTexture;
		SDFOuterTexture.Initialize2D(TEXT("ToonShadePaint.SDFOuterTexture"), GPixelFormats[PF_FloatRGBA].BlockBytes, Resolution, Resolution, PF_FloatRGBA, TextureCreateFlags);

		FRWByteAddressBuffer MaxDistanceBuffer;
		MaxDistanceBuffer.Initialize(RHICmdList, TEXT("ToonShadePaint.MaxDistanceBuffer"), sizeof(int32) * 1, BUF_ShaderResource | BUF_UnorderedAccess);

		FTextureRWBuffer SDFNormalizedTexture;
		Initialize2DArray(RHICmdList, SDFNormalizedTexture, TEXT("ToonShadePaint.SDFNormalizedTexture"), GPixelFormats[PF_R32_FLOAT].BlockBytes, Resolution, Resolution, NumSeedTextures, PF_R32_FLOAT, TextureCreateFlags);

		FTextureRWBuffer ShadowThresholdTexture;
		ShadowThresholdTexture.Initialize2D(TEXT("SDF.ShadowThresholdTexture"), GPixelFormats[PF_A32B32G32R32F].BlockBytes, Resolution, Resolution, PF_A32B32G32R32F, TextureCreateFlags);

		FTextureRWBuffer OutputShadowThresholdTexture;
		OutputShadowThresholdTexture.Initialize2D(TEXT("SDF.OutputShadowThresholdTexture"), GPixelFormats[PixelFormat].BlockBytes, Resolution, Resolution, PixelFormat, TextureCreateFlags);

		// いつかAsnycしたいからPositionTextureとPositionTextureの寿命を切り離し
		{
			RHICmdList.Transition(FRHITransitionInfo(SeedFlagsTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
			RHICmdList.Transition(FRHITransitionInfo(PositionTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

			int32 LayerIndex = 0;
			for (UTextureRenderTarget2D* SeedTexture : InSeedTextures)
			{
				if (IsValid(SeedTexture))
				{
					TShaderMapRef<FSetupSeedFlagsCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
					SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
					SetShaderParametersLegacyCS(
						RHICmdList,
						ComputeShader,
						LayerIndex++,
						SeedTexture->GetResource()->TextureRHI,
						SeedFlagsTexture.UAV);
					DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
					UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);
				}
			}

			{
				TShaderMapRef<FSetupPosCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					InPositionTexture->GetResource()->TextureRHI,
					PositionTexture.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);
			}

			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);  // DX12はAsyncComputeなので都度叩いて安牌

			RHICmdList.Transition(FRHITransitionInfo(SeedFlagsTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));
			RHICmdList.Transition(FRHITransitionInfo(PositionTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));

			// 理想はここで終わらせて裏で頑張ってもらう
			// Signal->Trigger();
		}

		RHICmdList.Transition(FRHITransitionInfo(SDFNormalizedTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

		for (int32 Index = 0; Index < NumSeedTextures; ++Index)
		{
			RHICmdList.Transition(FRHITransitionInfo(SDFInnerTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
			RHICmdList.Transition(FRHITransitionInfo(SDFOuterTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

			{
				TShaderMapRef<FDistanceMapSetupCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					Index,
					SeedFlagsTexture.SRV,
					SDFInnerTexture.UAV,
					SDFOuterTexture.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}

			for (int32 Radius = 1; Radius <= MaxRadius; ++Radius)
			{
				FDistanceMapIterCS::FPermutationDomain PermutationVector;
				PermutationVector.Set<FDistanceMapIterCS::FFlip>(Radius % 2 == 0);
				TShaderMapRef<FDistanceMapIterCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					Index,
					TextureSize,
					Radius,
					SeedFlagsTexture.SRV,
					PositionTexture.SRV,
					SDFInnerTexture.UAV,
					SDFOuterTexture.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}

			RHICmdList.Transition(FRHITransitionInfo(SDFInnerTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));
			RHICmdList.Transition(FRHITransitionInfo(SDFOuterTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));

			RHICmdList.Transition(FRHITransitionInfo(MaxDistanceBuffer.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

			{
				RHICmdList.ClearUAVUint(MaxDistanceBuffer.UAV, FUintVector4(0, 0, 0, 0));
				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}

			{
				FSDFCalcCS::FPermutationDomain PermutationVector;
				PermutationVector.Set<FSDFCalcCS::FFlip>(MaxRadius % 2 == 0);
				TShaderMapRef<FSDFCalcCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					Index,
					TextureSize,
					SeedFlagsTexture.SRV,
					PositionTexture.SRV,
					SDFInnerTexture.SRV,
					SDFOuterTexture.SRV,
					SDFNormalizedTexture.UAV,
					MaxDistanceBuffer.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}

			RHICmdList.Transition(FRHITransitionInfo(MaxDistanceBuffer.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

			{
				TShaderMapRef<FSDFNormalizedCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					Index,
					MaxDistanceBuffer.SRV,  // Readback面倒だからSRV
					SDFNormalizedTexture.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}
		}

		RHICmdList.Transition(FRHITransitionInfo(SDFNormalizedTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));

		RHICmdList.Transition(FRHITransitionInfo(ShadowThresholdTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

		{
			RHICmdList.ClearUAVFloat(ShadowThresholdTexture.UAV, FVector4f(0.0f, 0.0f, 0.0f, 0.0f));
			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
		}

		for (int32 Index = 0; Index < NumSeedTextures - 1; ++Index)
		{
			float Start = InvNumGradients * Index;
			float End = InvNumGradients * (Index + 1);
			{
				FSDFBlendCS::FPermutationDomain PermutationVector;
				PermutationVector.Set<FSDFBlendCS::FFlip>(Index % 2 == 0);
				TShaderMapRef<FSDFBlendCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
				SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
				SetShaderParametersLegacyCS(
					RHICmdList,
					ComputeShader,
					Start,
					End,
					Index,
					SeedFlagsTexture.SRV,
					SDFNormalizedTexture.SRV,
					ShadowThresholdTexture.UAV);
				DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
				UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

				RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
			}
		}

		RHICmdList.Transition(FRHITransitionInfo(ShadowThresholdTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));

		RHICmdList.Transition(FRHITransitionInfo(OutputShadowThresholdTexture.UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));

		{
			FShadowThresholdCS::FPermutationDomain PermutationVector;
			PermutationVector.Set<FShadowThresholdCS::FFlip>(NumSeedTextures % 2 == 0);
			TShaderMapRef<FShadowThresholdCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
			SetComputePipelineState(RHICmdList, ComputeShader.GetComputeShader());
			SetShaderParametersLegacyCS(
				RHICmdList,
				ComputeShader,
				SeedFlagsTexture.SRV,
				ShadowThresholdTexture.SRV,
				OutputShadowThresholdTexture.UAV);
			DispatchComputeShader(RHICmdList, ComputeShader.GetShader(), ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
			UnsetShaderParametersLegacyCS(RHICmdList, ComputeShader);

			RHICmdList.ImmediateFlush(EImmediateFlushType::DispatchToRHIThread);
		}

		RHICmdList.Transition(FRHITransitionInfo(OutputShadowThresholdTexture.UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVMask));

		{
			FRHITexture* SrcTexture = OutputShadowThresholdTexture.Buffer;
			FRHITexture* DstTexture = OutShadowThresholdMapTexture->GetResource()->TextureRHI;
			RHICmdList.Transition(FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::CopySrc));
			RHICmdList.Transition(FRHITransitionInfo(DstTexture, ERHIAccess::Unknown, ERHIAccess::CopyDest));
			RHICmdList.CopyTexture(SrcTexture, DstTexture, {});

			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);
		}

		Signal->Trigger();
	});

	// AsyncにするとOutShadowThresholdMapTextureの書き込み面倒だから終わるまで待つ
	Signal->Wait();
	FGenericPlatformProcess::ReturnSynchEventToPool(Signal);

	const double EndTime = FPlatformTime::Seconds();
	const double ElapsedTime = EndTime - StartTime;

	UE_LOG(LogToonShadePaint, Display, TEXT("CreateShadowThresholdMap: %f"), ElapsedTime);
}

void UToonShadePaintBlueprintLibrary::LayerSort(TArray<AToonShadeCaptureTargetActor*>& InValues)
{
	InValues.Sort([](const AToonShadeCaptureTargetActor& A, const AToonShadeCaptureTargetActor& B)
	{
		return A.Layer < B.Layer;
	});
}
