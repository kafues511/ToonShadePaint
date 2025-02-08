// Copyright © 2024-2025 kafues511 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ToonShadePaintBlueprintLibrary.generated.h"

TOONSHADEPAINT_API DECLARE_LOG_CATEGORY_EXTERN(LogToonShadePaint, Log, All);

class AToonShadeCaptureTargetActor;

/**
 * 
 */
UCLASS()
class TOONSHADEPAINT_API UToonShadePaintBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ToonShadePaint", meta = (WorldContext = "WorldContextObject"))
	static void CreateShadowThresholdMap(
		UObject* WorldContextObject,
		TArray<UTextureRenderTarget2D*> InSeedTextures,
		UTextureRenderTarget2D* InPositionTexture,
		int32 MaxRadius,
		UTextureRenderTarget2D* OutShadowThresholdMapTexture);

	UFUNCTION(BlueprintCallable, Category = "ToonShadePaint")
	static void LayerSort(UPARAM(ref) TArray<AToonShadeCaptureTargetActor*>& InValues);
};
