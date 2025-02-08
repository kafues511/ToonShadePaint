// Copyright © 2024-2025 kafues511 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ToonShadePaintActor.generated.h"

class UMaterialParameterCollection;
class UStaticMesh;
class UStaticMeshComponent;
class UCapsuleComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EPaintType : uint8
{
	/** なにもしない */
	None,
	/** 塗り潰し */
	Fill,
	/** マスク */
	Mask,
};

UENUM(BlueprintType)
enum class EInvalidType : uint8
{
	/** なにもしない */
	None,
	/** 塗り潰し */
	Fill,
	/** マスク */
	Mask,
};

UENUM(BlueprintType)
enum class EPaintShapeType : uint8
{
	/** 球 */
	Sphere,
	/** ボックス */
	Box,
	/** 円柱 */
	Cylinder,
	/** 円錐 */
	Cone,
	/** カプセル */
	Capsule,
};

UCLASS(hidecategories = (Rendering, Replication, Collision, HLOD, Physics, Actor, Networking, Input, DataLayers, Cooking, LevelInstance))
class TOONSHADEPAINT_API AToonShadeShapeActor : public AActor
{
	GENERATED_BODY()
	
public:
	AToonShadeShapeActor(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

public:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/**  */
	void Initialize();

	/**
	 * InShapeTypeに紐づいたUStaticMeshを取得
	 * @param InShapeType TEXT
	 * @return TObjectPtr<UStaticMesh> TEXT
	 */
	TObjectPtr<UStaticMesh> GetShapeMesh(EPaintShapeType InShapeType) const;

	/**
	 * InShapeTypeに紐づいたUStaticMeshのスケールを取得
	 * @param InShapeType TEXT
	 * @return TOptional<FVector> TEXT
	 */
	TOptional<FVector> GetShapeScale(EPaintShapeType InShapeType) const;

public:
	/**
	 * 陰の有効性
	 * 無効にしても指定されたレイヤーは空き状態にはなりません。
	 * 不要な場合は有効性を無効ではなく、レベルから削除してください。
	 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	uint32 bEnabled : 1;

	/** デバッグ用に自由に指定可能な表示名 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shade Painter")
	FName DebugName;

	/**
	 * 陰の描画順
	 * 値が小さい順から書き込み・上書きしていきます。
	 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (ClampMin = "0", ClampMax = "63", UIMin = "0", UIMax = "63"))
	int32 Layer;

	/** 陰の塗り方 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	EPaintType PaintType;

	/**
	 * 無効の塗り方
	 * 明暗の距離計算から除外したい箇所に指定します。
	 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	EInvalidType InvalidType;

	/** 陰の形状 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	EPaintShapeType ShapeType;

	/** カプセルの高さ */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "ShapeType == EPaintShapeType::Capsule"))
	float Height;

	/** カプセルの半径 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "ShapeType == EPaintShapeType::Capsule"))
	float Radius;

	/** 扇形マスクの有効性 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	uint32 bMask : 1;

	/** マスク角度 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "bMask", ClampMin = "0", ClampMax = "360", UIMin = "0", UIMax = "360"))
	float MaskAngle;

	/**
	 * マスクの強度
	 * マスク角度を180以外で各ベクトル強度を編集すると、それっぽい曲線が簡単に作れる。
	 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "bMask"))
	FVector MaskIntensity;

	/** マスクの向き */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "bMask"))
	FVector MaskAxis;

	/** フリップの有効性 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter")
	uint32 bFlip : 1;

	/** フリップのピボット座標 */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "bFlip"))
	FVector FlipCenter;

	/** フリップの向き */
	UPROPERTY(EditAnywhere, Category = "Shade Painter", meta = (EditCondition = "bFlip"))
	FVector FlipAxis;

private:
	/** パラメータの格納先のMPC */
	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> MPC;

	/** 球のデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	/** ボックスのデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> BoxMesh;

	/** 円柱のデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	/** 円錐のデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> ConeMesh;

	/** 各形状のデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/** カプセルのデバッグ描画 */
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> WireframeMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> FillMaterial;

private:
	int32 CachedLayer;
	EPaintShapeType CachedShapeType;
	FVector CachedScale;

private:
	friend class UToonShadePaintSubsystem;
};
