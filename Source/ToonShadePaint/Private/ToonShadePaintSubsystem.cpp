// Copyright © 2024-2025 kafues511 All Rights Reserved.

#include "ToonShadePaintSubsystem.h"
#include "Algo/IndexOf.h"
#include "ToonShadePaintActor.h"

UToonShadePaintSubsystem::UToonShadePaintSubsystem()
	: Super()
{
	UsedLayerList.SetNum(kMaxLayer);
}

void UToonShadePaintSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UToonShadePaintSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

ETickableTickType UToonShadePaintSubsystem::GetTickableTickType() const
{
	return ETickableTickType::Never;
}

bool UToonShadePaintSubsystem::IsTickable() const
{
	return false;
}

bool UToonShadePaintSubsystem::IsTickableInEditor() const
{
	return false;
}

void UToonShadePaintSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UToonShadePaintSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UToonShadePaintSubsystem, STATGROUP_Tickables);
}

int32 UToonShadePaintSubsystem::GetFreeLayer(const TObjectPtr<AToonShadeShapeActor> InTestShadePaint)
{
	{
		const int32 Layer = Algo::IndexOfByPredicate(UsedLayerList, [InTestShadePaint](const FToonShadePaintLayer& ToonShadePaintLayer) { return ToonShadePaintLayer.Owner == InTestShadePaint; });
		if (Layer != INDEX_NONE)
		{
			const FToonShadePaintLayer& ToonShadePaintLayer = UsedLayerList[Layer];
			if (ToonShadePaintLayer.bUsed && IsValid(ToonShadePaintLayer.Owner))
			{
				return Layer;
			}
		}
	}

	for (int32 Layer = 0; Layer < kMaxLayer; ++Layer)
	{
		if (!UsedLayerList[Layer].bUsed)
		{
			UsedLayerList[Layer].bUsed = true;
			UsedLayerList[Layer].Owner = InTestShadePaint;
			return Layer;
		}
		if (!IsValid(UsedLayerList[Layer].Owner))
		{
			UsedLayerList[Layer].bUsed = true;
			UsedLayerList[Layer].Owner = InTestShadePaint;
			return Layer;
		}
	}
	return INDEX_NONE;
}

int32 UToonShadePaintSubsystem::SetLayer(int32 InLayer, const TObjectPtr<AToonShadeShapeActor> InTestShadePaint)
{
	UsedLayerList[InLayer].bUsed = true;
	UsedLayerList[InLayer].Owner = InTestShadePaint;
	return InLayer;
}

void UToonShadePaintSubsystem::ChangeLayer(int32 InPrevLayer, int32 InNewLayer, const TObjectPtr<AToonShadeShapeActor> InTestShadePaint)
{
	if (!IsValidLayer(InPrevLayer) || !IsValidLayer(InNewLayer))
	{
		return;  // 不正なレイヤー
	}

	if (UsedLayerList[InPrevLayer].Owner != InTestShadePaint)
	{
		return;  // 関数の発行者が不正
	}

	// 削除
	UsedLayerList[InPrevLayer].bUsed = false;
	UsedLayerList[InPrevLayer].Owner = nullptr;

	// 使用中の場合はレイヤー交換
	if (UsedLayerList[InNewLayer].bUsed)
	{
		// 解放済みのShadePaintが残留しているだけならスルー
		if (AToonShadeShapeActor* Owner = UsedLayerList[InNewLayer].Owner; IsValid(Owner))
		{
			UsedLayerList[InPrevLayer].bUsed = true;
			UsedLayerList[InPrevLayer].Owner = Owner;

			// プロパティ変更を発火させるほどでもないので強制変更
			// TODO: CB更新はしないとダメじゃんね
			Owner->CachedLayer = Owner->Layer = InPrevLayer;
		}
	}

	// 新規
	UsedLayerList[InNewLayer].bUsed = true;
	UsedLayerList[InNewLayer].Owner = InTestShadePaint;
}
