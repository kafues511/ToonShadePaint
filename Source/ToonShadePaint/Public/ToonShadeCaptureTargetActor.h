// Copyright © 2024-2025 kafues511 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ToonShadeCaptureTargetActor.generated.h"

class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;
class USkeletalMesh;

UENUM(BlueprintType)
enum class EResolutionType : uint8
{
	/** Seed */
	Seed,
	/** Position */
	Position,
};

UENUM(BlueprintType)
enum class EToonShadeResolution : uint8
{
	/** 128 */
	Resolution_128 UMETA(DisplayName = "128"),
	/** 256 */
	Resolution_256 UMETA(DisplayName = "256"),
	/** 512 */
	Resolution_512 UMETA(DisplayName = "512"),
	/** 1024 */
	Resolution_1024 UMETA(DisplayName = "1024"),
	/** 2048 */
	Resolution_2048 UMETA(DisplayName = "2048"),
	/** 4096 */
	//Resolution_4096 UMETA(DisplayName = "4096"),
	/** 8192 */
	//Resolution_8192 UMETA(DisplayName = "8192"),
	/** 16384 */
	//Resolution_16384 UMETA(DisplayName = "16384"),
};

USTRUCT(BlueprintType)
struct FCaptureMaterial
{
	GENERATED_BODY()

	/**  */
	UPROPERTY(VisibleAnywhere, Category = "Default")
	FName MaterialSlotName;

	/**  */
	UPROPERTY(EditAnywhere, Category = "Default")
	uint32 bEnabled : 1;

	/**  */
	UPROPERTY(EditAnywhere, Category = "Default", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	int32 CoordinateIndex;

	/**  */
	UPROPERTY(EditAnywhere, Category = "Default")
	TObjectPtr<UTexture2D> BaseColorTexture;

	FCaptureMaterial()
		: MaterialSlotName(NAME_None)
		, bEnabled(false)
		, CoordinateIndex(1)
		, BaseColorTexture(nullptr)
	{
	}

	FCaptureMaterial(FName InMaterialSlotName, TObjectPtr<UTexture2D> InBaseColorTexture)
		: MaterialSlotName(InMaterialSlotName)
		, bEnabled(false)
		, CoordinateIndex(1)
		, BaseColorTexture(InBaseColorTexture)
	{
	}
};

UCLASS(hidecategories = (Rendering, Replication, Collision, HLOD, Physics, Actor, Networking, Input, DataLayers, Cooking, LevelInstance))
class TOONSHADEPAINT_API AToonShadeCaptureTargetActor : public AActor
{
	GENERATED_BODY()
	
public:
	AToonShadeCaptureTargetActor(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

public:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UFUNCTION(BlueprintCallable, Category = "Shade Painter")
	void CaptureSetup();

	UFUNCTION(BlueprintCallable, Category = "Shade Painter")
	void Capture();

public:
	/**
	 * 有効性
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shade Painter")
	uint32 bEnabled : 1;

	/** デバッグ用に自由に指定可能な表示名 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shade Painter")
	FName DebugName;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shade Painter")
	int32 Layer;

	/**  */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	EToonShadeResolution Resolution;

	/** リソースの種類 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shade Painter")
	EResolutionType ResolutionType;

	/**  */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	TObjectPtr<USkeletalMesh> SkeletalMeshAsset;

	/**  */
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Shade Painter")
	TArray<FCaptureMaterial> CaptureMaterials;

public:
	/**  */
	UPROPERTY(BlueprintReadOnly, Category = "Shade Painter")
	TObjectPtr<UTextureRenderTarget2D> TextureRenderTarget;

private:
	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> CachedSkeletalMeshAsset;
};
