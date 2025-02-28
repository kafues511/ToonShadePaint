// Copyright © 2024-2025 kafues511 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ToonShadePaintSubsystem.generated.h"

class AToonShadeShapeActor;

USTRUCT()
struct TOONSHADEPAINT_API FToonShadePaintLayer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool bUsed;

	UPROPERTY()
	TObjectPtr<AToonShadeShapeActor> Owner;

	FToonShadePaintLayer()
		: bUsed(false)
		, Owner(nullptr)
	{
	}

	FToonShadePaintLayer(bool bInUsed, TObjectPtr<AToonShadeShapeActor> InOwner)
		: bUsed(bInUsed)
		, Owner(InOwner)
	{
	}
};

/**
 * 形状のレイヤー管理用
 */
UCLASS()
class TOONSHADEPAINT_API UToonShadePaintSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UToonShadePaintSubsystem();
	virtual ~UToonShadePaintSubsystem() = default;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	/**
	 * UShadePaintSubsystemを取得
	 * @param World World
	 * @return UToonShadePaintSubsystem
	 */
	static inline UToonShadePaintSubsystem* GetCurrent(const UWorld* World)
	{
		return UWorld::GetSubsystem<UToonShadePaintSubsystem>(World);
	}

	/**
	 * 有効なレイヤーか判定
	 * @param InLayer レイヤー
	 * @return bool レイヤーが有効な場合はtrueを返します。
	 */
	static inline bool IsValidLayer(int32 InLayer)
	{
		return (InLayer >= 0 && InLayer < kMaxLayer);
	}

public:
	/**
	 * 空いているレイヤーを取得
	 * 空いているレイヤーが存在しない場合はINDEX_NONEを返します。
	 * @param InTestShadePaint 
	 * @return int32 
	 */
	int32 GetFreeLayer(const TObjectPtr<AToonShadeShapeActor> InTestShadePaint);

	/**
	 * テキスト
	 * @param InLayer テキスト
	 * @param InTestShadePaint テキスト
	 * @return int32 InLayer
	 */
	int32 SetLayer(int32 InLayer, const TObjectPtr<AToonShadeShapeActor> InTestShadePaint);

	/**
	 * レイヤーの変更
	 * InNewLayerが使用中の場合は、そいつとレイヤーを交換します。
	 * @param InPrevLayer 変更前に所属していたレイヤー
	 * @param InNewLayer 変更後に所属するレイヤー
	 * @param InTestShadePaint 
	 */
	void ChangeLayer(int32 InPrevLayer, int32 InNewLayer, const TObjectPtr<AToonShadeShapeActor> InTestShadePaint);

public:
	/** 最大レイヤー数 */
	static constexpr int32 kMaxLayer = 64;

private:
	/**  */
	UPROPERTY()
	TArray<FToonShadePaintLayer> UsedLayerList;
};
