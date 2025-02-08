// Copyright © 2024-2025 kafues511 All Rights Reserved.

#include "ToonShadePaintActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "ToonShadePaintSubsystem.h"

AToonShadeShapeActor::AToonShadeShapeActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEnabled(false)
	, DebugName(NAME_None)
	, Layer(INDEX_NONE)
	, PaintType(EPaintType::Fill)
	, InvalidType(EInvalidType::None)
	, ShapeType(EPaintShapeType::Capsule)
	, Height(2.0f)
	, Radius(1.0f)
	, bMask(false)
	, MaskAngle(360.0f)
	, MaskIntensity(FVector(1.0, 1.0, 0.0))
	, MaskAxis(FVector::YAxisVector)
	, bFlip(false)
	, FlipAxis(FVector::XAxisVector)
	, CachedLayer(INDEX_NONE)
	, CachedScale(FVector::OneVector)
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->bCastStaticShadow = false;
	StaticMeshComponent->bCastDynamicShadow = false;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->SetupAttachment(RootComponent);
	CapsuleComponent->bCastStaticShadow = false;
	CapsuleComponent->bCastDynamicShadow = false;
	CapsuleComponent->ShapeColor = FColor::Red;

	MPC = Cast<UMaterialParameterCollection>(StaticLoadObject(UMaterialParameterCollection::StaticClass(), NULL, TEXT("/ToonShadePaint/Materials/MPC_ShapeParameters")));

	SphereMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Sphere")));
	BoxMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Cube")));
	CylinderMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Cylinder")));
	ConeMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Cone")));

	WireframeMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/ToonShadePaint/Materials/M_Wireframe")));
	FillMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/ToonShadePaint/Materials/M_Fill")));
}

void AToonShadeShapeActor::BeginPlay()
{
	Super::BeginPlay();

	Initialize();
}

void AToonShadeShapeActor::Destroyed()
{
	if (UWorld* World = GetWorld(); IsValid(World) && IsValid(MPC))
	{
		if (UMaterialParameterCollectionInstance* MPCInstance = World->GetParameterCollectionInstance(MPC); IsValid(MPCInstance))
		{
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d00"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d01"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d02"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d03"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d04"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d05"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d06"), Layer), FLinearColor::Transparent);
			MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d07"), Layer), FLinearColor::Transparent);
		}
	}

	Super::Destroyed();
}

void AToonShadeShapeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Initialize();
}

#if WITH_EDITOR
void AToonShadeShapeActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	const FName PropertyName = PropertyThatWillChange ? PropertyThatWillChange->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeShapeActor, Layer))
	{
		CachedLayer = Layer;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeShapeActor, ShapeType))
	{
		CachedShapeType = ShapeType;
		if (ShapeType != EPaintShapeType::Capsule)
		{
			CachedScale = GetActorScale3D();
		}
	}

	Super::PreEditChange(PropertyThatWillChange);
}

void AToonShadeShapeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeShapeActor, Layer))
	{
		if (CachedLayer != Layer)
		{
			if (UToonShadePaintSubsystem* Subsystem = UToonShadePaintSubsystem::GetCurrent(GetWorld()); IsValid(Subsystem))
			{
				Subsystem->ChangeLayer(CachedLayer, Layer, this);
				CachedLayer = Layer;
			}
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeShapeActor, ShapeType))
	{
		if (CachedShapeType != ShapeType)
		{
			if (CachedShapeType != EPaintShapeType::Capsule && ShapeType == EPaintShapeType::Capsule)
			{
				// カプセルに切り替えたらスケールをOneに強制変更
				// UCapsuleComponentの都合上
				SetActorScale3D(FVector::OneVector);
			}
			else if (CachedShapeType == EPaintShapeType::Capsule && ShapeType != EPaintShapeType::Capsule)
			{
				// カプセルから切り替えたらキャッシュしたスケールに戻す
				// シリアライズ対象外だからエディタ閉じると消失するけど特に困ってない
				SetActorScale3D(CachedScale);
			}

			TObjectPtr<UStaticMesh> ShapeMesh = GetShapeMesh(ShapeType);
			if (IsValid(ShapeMesh) && IsValid(StaticMeshComponent) && IsValid(WireframeMaterial) && IsValid(FillMaterial))
			{
				StaticMeshComponent->SetStaticMesh(ShapeMesh);
				if (TOptional<FVector> ShapeScale = GetShapeScale(ShapeType); ShapeScale.IsSet())
				{
					StaticMeshComponent->SetRelativeScale3D(*ShapeScale);
				}

				const int32 NumMaterials = StaticMeshComponent->GetNumMaterials();
				for (int32 ElementIndex = 0; ElementIndex < NumMaterials; ++ElementIndex)
				{
					StaticMeshComponent->SetMaterial(ElementIndex, WireframeMaterial);
				}

				StaticMeshComponent->SetOverlayMaterial(FillMaterial);
			}

			if (ShapeType == EPaintShapeType::Capsule)
			{
				CapsuleComponent->SetCapsuleHalfHeight(Height, false);
				CapsuleComponent->SetCapsuleRadius(Radius, true);
				CapsuleComponent->SetVisibility(true);

				if (IsValid(StaticMeshComponent))
				{
					StaticMeshComponent->SetVisibility(false);
				}
			}
			else
			{
				CapsuleComponent->SetVisibility(false);

				if (IsValid(StaticMeshComponent))
				{
					StaticMeshComponent->SetVisibility(true);
				}
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AToonShadeShapeActor::Initialize()
{
	UToonShadePaintSubsystem* Subsystem = UToonShadePaintSubsystem::GetCurrent(GetWorld());
	if (!IsValid(Subsystem))
	{
		return;
	}

	if (Layer == INDEX_NONE)
	{
		CachedLayer = Layer = Subsystem->GetFreeLayer(this);  // 新規配置
	}
	else if (CachedLayer != Layer)
	{
		if (CachedLayer == INDEX_NONE)
		{

			CachedLayer = Layer = Subsystem->SetLayer(Layer, this);  // アーカイブ読込
		}
		else
		{
			CachedLayer = Layer = Subsystem->GetFreeLayer(this);  // コピペ配置
		}
	}

	if (ShapeType == EPaintShapeType::Capsule)
	{
		CapsuleComponent->SetCapsuleHalfHeight(Height, false);
		CapsuleComponent->SetCapsuleRadius(Radius, true);
	}

	const FVector Location = GetActorLocation();
	const FQuat   Rotation = GetActorQuat();
	const FVector Scale = ShapeType == EPaintShapeType::Capsule ? FVector(Radius, Radius, Height) : GetActorScale3D();

	const FVector AxisX = Rotation.GetAxisX();
	const FVector AxisY = Rotation.GetAxisY();
	const FVector AxisZ = Rotation.GetAxisZ();

	const FVector SafeMaskAxis = bMask ? MaskAxis : FVector::YAxisVector;
	const float   SafeMaskAngle = bMask ? MaskAngle : 360.0f;

	const FVector SafeFlipCenter = bFlip ? FlipCenter : FVector::ZeroVector;
	const FVector SafeFlipAxis = bFlip ? FlipAxis : FVector::ZeroVector;

	const bool bIsZeroDiv = Scale.GetAbsMin() < FLT_EPSILON;

	const FLinearColor AxisXAndCenterX(AxisX.X, AxisX.Y, AxisX.Z, Location.X);
	const FLinearColor AxisYAndCenterY(AxisY.X, AxisY.Y, AxisY.Z, Location.Y);
	const FLinearColor AxisZAndCenterZ(AxisZ.X, AxisZ.Y, AxisZ.Z, Location.Z);
	const FLinearColor ExtentAndPad(Scale.X, Scale.Y, Scale.Z, 0.0f);
	const FLinearColor EnabledAndTypes(bEnabled && !bIsZeroDiv ? 1.0f : 0.0f, static_cast<float>(ShapeType), static_cast<float>(PaintType), static_cast<float>(InvalidType));
	const FLinearColor MaskAxisAndMaskAngle(SafeMaskAxis.X, SafeMaskAxis.Y, SafeMaskAxis.Z, SafeMaskAngle);
	const FLinearColor MaskIntensityAndPad(MaskIntensity.X, MaskIntensity.Y, MaskIntensity.Z, 0.0f);
	const FLinearColor FlipCenterAndPad(SafeFlipCenter.X, SafeFlipCenter.Y, SafeFlipCenter.Z, 0.0f);
	const FLinearColor FlipAxisAndPad(SafeFlipAxis.X, SafeFlipAxis.Y, SafeFlipAxis.Z, 0.0f);

	UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC);
	if (IsValid(MPCInstance))
	{
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d00"), Layer), AxisXAndCenterX);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d01"), Layer), AxisYAndCenterY);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d02"), Layer), AxisZAndCenterZ);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d03"), Layer), ExtentAndPad);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d04"), Layer), EnabledAndTypes);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d05"), Layer), MaskAxisAndMaskAngle);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d06"), Layer), MaskIntensityAndPad);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d07"), Layer), FlipCenterAndPad);
		MPCInstance->SetVectorParameterValue(*FString::Printf(TEXT("Params%02d08"), Layer), FlipAxisAndPad);
	}
}

TObjectPtr<UStaticMesh> AToonShadeShapeActor::GetShapeMesh(EPaintShapeType InShapeType) const
{
	switch (InShapeType)
	{
	case EPaintShapeType::Sphere:
		return SphereMesh;
	case EPaintShapeType::Box:
		return BoxMesh;
	case EPaintShapeType::Cylinder:
		return CylinderMesh;
	case EPaintShapeType::Cone:
		return ConeMesh;
	case EPaintShapeType::Capsule:
	default:
		return nullptr;
	}
}

TOptional<FVector> AToonShadeShapeActor::GetShapeScale(EPaintShapeType InShapeType) const
{
	// NOTE: シェーダー側を真として適当に調整している。
	switch (InShapeType)
	{
	case EPaintShapeType::Sphere:
		return FVector(0.0201f);
	case EPaintShapeType::Box:
		return FVector(0.02f);
	case EPaintShapeType::Cylinder:
		return FVector(0.02f);
	case EPaintShapeType::Cone:
		return FVector(0.02f, 0.02f, 0.01f);
	case EPaintShapeType::Capsule:
	default:
		return NullOpt;
	}
}
