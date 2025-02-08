// Copyright © 2024-2025 kafues511 All Rights Reserved.

#include "ToonShadeCaptureTargetActor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"

static int32 ToInt32(EToonShadeResolution ToonShadeResolution)
{
	switch (ToonShadeResolution)
	{
	case EToonShadeResolution::Resolution_128:
		return 128;
	case EToonShadeResolution::Resolution_256:
		return 256;
	case EToonShadeResolution::Resolution_512:
		return 512;
	case EToonShadeResolution::Resolution_1024:
		return 1024;
	case EToonShadeResolution::Resolution_2048:
		return 2048;
	//case EToonShadeResolution::Resolution_4096:
	//	return 4096;
	//case EToonShadeResolution::Resolution_8192:
	//	return 8192;
	//case EToonShadeResolution::Resolution_16384:
	//	return 16384;
	default:
		return 2048;
	}
}

AToonShadeCaptureTargetActor::AToonShadeCaptureTargetActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bEnabled(true)
	, Resolution(EToonShadeResolution::Resolution_2048)
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->SetupAttachment(RootComponent);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->BoundsScale = 100.0f;
	SkeletalMeshComponent->SetupAttachment(RootComponent);

	CaptureSetup();
}

void AToonShadeCaptureTargetActor::BeginPlay()
{
	Super::BeginPlay();	
}

void AToonShadeCaptureTargetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UMaterialInterface* DisableMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/ToonShadePaint/Materials/M_Disable")));
	UMaterialInterface* ToonShadePaintMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/ToonShadePaint/Materials/M_ToonShadePaint")));

	const int32 NumMaterials = CaptureMaterials.Num();

	for (int32 ElementIndex = 0; ElementIndex < NumMaterials; ++ElementIndex)
	{
		const FCaptureMaterial& CaptureMaterial = CaptureMaterials[ElementIndex];

		UMaterialInterface* Material = CaptureMaterial.bEnabled ? ToonShadePaintMaterial : DisableMaterial;
		if (!IsValid(Material))
		{
			continue;
		}

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
		if (!IsValid(MID))
		{
			MID = SkeletalMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(ElementIndex, Material);
		}

		if (IsValid(CaptureMaterial.BaseColorTexture))
		{
			MID->SetTextureParameterValue(TEXT("BaseColor"), CaptureMaterial.BaseColorTexture);
		}
		MID->SetScalarParameterValue(TEXT("CoordinateIndex"), static_cast<float>(CaptureMaterial.CoordinateIndex));
	}
}

#if WITH_EDITOR
void AToonShadeCaptureTargetActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	const FName PropertyName = PropertyThatWillChange ? PropertyThatWillChange->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeCaptureTargetActor, SkeletalMeshAsset))
	{
		CachedSkeletalMeshAsset = SkeletalMeshAsset;
	}

	Super::PreEditChange(PropertyThatWillChange);
}

void AToonShadeCaptureTargetActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AToonShadeCaptureTargetActor, SkeletalMeshAsset))
	{
		if (CachedSkeletalMeshAsset != SkeletalMeshAsset)
		{
			UTexture2D* DummyTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("/ToonShadePaint/Textures/T_White")));

			CaptureMaterials.Empty();

			for (const FSkeletalMaterial& Material : SkeletalMeshAsset->GetMaterials())
			{
				CaptureMaterials.Add(FCaptureMaterial(Material.MaterialSlotName, DummyTexture));
			}

			SkeletalMeshComponent->SetSkeletalMesh(SkeletalMeshAsset);
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AToonShadeCaptureTargetActor::CaptureSetup()
{
	const ETextureRenderTargetFormat TextureFormat = (ResolutionType == EResolutionType::Seed) ? RTF_RGBA8 : RTF_RGBA32f;
	int32 SizeX = ToInt32(Resolution);

	TextureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), ToInt32(Resolution), ToInt32(Resolution), TextureFormat, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
	if (!IsValid(TextureRenderTarget))
	{
		return;
	}

	SkeletalMeshComponent->CastShadow = false;

	int32 NumMaterials = SkeletalMeshComponent->GetNumMaterials();

	for (int32 ElementIndex = 0; ElementIndex < NumMaterials; ++ElementIndex)
	{
		UMaterialInterface* Material = SkeletalMeshComponent->GetMaterial(ElementIndex);
		if (!IsValid(Material))
		{
			continue;
		}

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
		if (!IsValid(MID))
		{
			MID = SkeletalMeshComponent->CreateAndSetMaterialInstanceDynamicFromMaterial(ElementIndex, Material);
		}
		
		MID->SetScalarParameterValue(TEXT("CaptureMode"), static_cast<float>(ResolutionType) + 1.0f);
		MID->SetScalarParameterValue(TEXT("Resolution"), static_cast<float>(SizeX));
		MID->SetVectorParameterValue(TEXT("Center"), GetActorLocation());
	}

	SceneCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	SceneCaptureComponent->SetRelativeRotation(FRotator(-90.0, 0.0, 270.0));

	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	SceneCaptureComponent->OrthoWidth = SizeX;
	SceneCaptureComponent->bAutoCalculateOrthoPlanes = false;

	SceneCaptureComponent->PostProcessBlendWeight = 0.0f;

	SceneCaptureComponent->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;

	SceneCaptureComponent->ShowOnlyActors.Reset();
	SceneCaptureComponent->ShowOnlyActors.Add(this);
}

void AToonShadeCaptureTargetActor::Capture()
{
	if (!IsValid(TextureRenderTarget))
	{
		return;
	}

	SceneCaptureComponent->TextureTarget = TextureRenderTarget;
	SceneCaptureComponent->CaptureScene();

	int32 NumMaterials = SkeletalMeshComponent->GetNumMaterials();

	for (int32 ElementIndex = 0; ElementIndex < NumMaterials; ++ElementIndex)
	{
		UMaterialInterface* Material = SkeletalMeshComponent->GetMaterial(ElementIndex);
		if (!IsValid(Material))
		{
			continue;
		}

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
		if (!IsValid(MID))
		{
			continue;
		}

		MID->SetScalarParameterValue(TEXT("CaptureMode"), 0.0f);
	}
}
