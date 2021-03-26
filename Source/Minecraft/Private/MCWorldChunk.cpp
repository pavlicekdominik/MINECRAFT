// All rights reserved Dominik Pavlicek


#include "MCWorldChunk.h"
#include <Materials/MaterialInstance.h>
#include <SimplexNoiseBPLibrary.h>

// Sets default values
AMCWorldChunk::AMCWorldChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	TempRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NewRoot"));
	SetRootComponent(TempRoot);

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceFinder_Snow(TEXT("/Game/0_MC/Materials/MI_Snow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceFinder_Grass(TEXT("/Game/0_MC/Materials/MI_Grass"));
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> MaterialInstanceFinder_Stone(TEXT("/Game/0_MC/Materials/MI_Stone"));
	MaterialsMapping.Add(0, MaterialInstanceFinder_Snow.Object);
	MaterialsMapping.Add(1, MaterialInstanceFinder_Grass.Object);
	MaterialsMapping.Add(2, MaterialInstanceFinder_Stone.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshFinder(TEXT("/Game/0_MC/Mesh/SM_MCCube"));
	BoxMesh = StaticMeshFinder.Object;

	for (int32 i = 0; i < (MaterialsMapping.Num()); i++)
	{

		UInstancedStaticMeshComponent* NewComp = CreateDefaultSubobject<UInstancedStaticMeshComponent>(FName(FString("instancedMesh").Append(FString::FormatAsNumber(i))));
		if (NewComp)
		{
			NewComp->AttachTo(GetRootComponent());

			InstancedBoxes.Add(NewComp);

			if (BoxMesh)
			{
				NewComp->SetStaticMesh(BoxMesh);

				if (MaterialsMapping.Contains(i))
				{
					NewComp->SetMaterial(0, MaterialsMapping.FindRef(i));
				}
			}
		}
	}

	Area = 16;
	Depth = 2;
	VoxelSize = 100;
	NoiseDensity = 0.001;
	NoiseScale = 5;
	_3DNoiseDensity = 0.01;
	_3DNoiseCutOff = 0.f;
	SnowTreshold = 600;
	GrassTreshold = 350;

	//SpawnWorldChunk();
}

void AMCWorldChunk::SpawnWorldChunk(class UStaticMesh* NewBoxMesh, int32 NewArea, int32 NewDepth, int32 NewVoxelSize, float NewNoiseDensity, int32 NewNoiseScale, float New3DNoiseDensity, float New3DNoiseCutOff)
{
	BoxMesh = NewBoxMesh;
	Area = NewArea;
	Depth = NewDepth;
	VoxelSize = NewVoxelSize;
	NoiseDensity = NewNoiseDensity;
	NoiseScale = NewNoiseScale;
	_3DNoiseDensity = New3DNoiseDensity;
	_3DNoiseCutOff = New3DNoiseCutOff;

	SpawnWorldChunk();
}

void AMCWorldChunk::SpawnWorldChunk()
{

	// TODO: How can I optimize this part?
	for (int32 i = (Area * (-1)); i < Area; i++)
	{
		float RandomSeed = FMath::RandRange(0.0001f, 0.02f);

		LocalVoxelPos_X = i;

		for (int32 j = (Area * (-1)); j < Area; j++)
		{
			LocalVoxelPos_Y = j;

			for (int32 k = (Depth * (-1)); k < Depth; k++)
			{
				LocalVoxelPos_Z = k;

				FVector TempVector = (GetActorLocation() / VoxelSize);
				WorldVoxelPos_X = TempVector.X + LocalVoxelPos_X; // (FMath::FloorToInt(TempVector.X) + LocalVoxelPos_X);
				WorldVoxelPos_Y = TempVector.Y + LocalVoxelPos_Y; //(FMath::FloorToInt(TempVector.Y) + LocalVoxelPos_Y);
				WorldVoxelPos_Z_PreNoise = TempVector.Z + LocalVoxelPos_Z; //(FMath::FloorToInt(TempVector.Z) + LocalVoxelPos_Z);

				// TODO: clean this a bit
				// In code the noise is instanced and static therefore nonprocedural, in BP the noise is not instanced and procedural - why and how to fix it?
				// Standard UE PerlinNoise that can be used instead of SimplexNoise
				// FMath::PerlinNoise2D(FVector2D((VoxelSize * WorldVoxelPos_X), (VoxelSize * WorldVoxelPos_Y)));

				USimplexNoiseBPLibrary* SimplexLibrary = NewObject<USimplexNoiseBPLibrary>();
				
				if(!SimplexLibrary) return;

				

				float TempZf1 = SimplexLibrary->SimplexNoise2D((VoxelSize * WorldVoxelPos_X) + RandomSeed, (VoxelSize * WorldVoxelPos_Y) + RandomSeed, NoiseDensity + RandomSeed);
				float TempZf2 = (NoiseScale * TempZf1);
				int32 TempZi1 = FMath::FloorToInt(TempZf2);

				int32 TempZi2 = ((TempZi1 - LocalVoxelPos_Z) * VoxelSize) + RandomSeed;

				WorldVoxelPos_Z_Noised = TempZi2; //(FMath::FloorToInt(SimplexNoise2D((VoxelSize * WorldVoxelPos_X), (VoxelSize * WorldVoxelPos_Y), NoiseDensity)) - LocalVoxelPos_Z) * VoxelSize;

				if (LocalVoxelPos_Z * (-1) < (Depth - FMath::RandRange(0, 1)))
				{
					if (Get3DNoiseZ(_3DNoiseCutOff, _3DNoiseDensity))
					{
						SpawnCubeInstance();
					}
				}
				else
				{
					if (Get3DNoiseZ(-0.5, 0.025))
					{
						SpawnCubeInstance();
					}
				}
			}
		}
	}
}

bool AMCWorldChunk::Get3DNoiseZ(const float InNoiseCutof, const float InNoiseDensity)
{

	int RandomSeed = FMath::RandRange(-33, 26);

	USimplexNoiseBPLibrary* SimplexLibrary = NewObject<USimplexNoiseBPLibrary>();

	if(!SimplexLibrary) 
	{
		return false;
	}
	return SimplexLibrary->SimplexNoise3D((VoxelSize * WorldVoxelPos_X) + RandomSeed, (VoxelSize * WorldVoxelPos_Y) + RandomSeed, (VoxelSize * WorldVoxelPos_Z_Noised) + RandomSeed, InNoiseCutof) <= NoiseDensity;
}

void AMCWorldChunk::SpawnCubeInstance()
{
	int32 activeIndex = 0;

	if(WorldVoxelPos_Z_Noised > SnowTreshold) 
	{
		activeIndex = 0;
	}
	else if (WorldVoxelPos_Z_Noised < GrassTreshold) 
	{
		activeIndex = 1;
	}
	else 
	{
		activeIndex = 2;
	}

	AddVoxelInstanceOfType(InstancedBoxes[activeIndex]);
}

void AMCWorldChunk::AddVoxelInstanceOfType(class UInstancedStaticMeshComponent* InstanceType)
{
	FTransform InstanceSpawnTransform = FTransform(FRotator(0), FVector((VoxelSize * LocalVoxelPos_X), (VoxelSize * LocalVoxelPos_Y), (WorldVoxelPos_Z_Noised)), FVector(1));
	
	if (InstanceType)
	{
		InstanceType->AddInstance(InstanceSpawnTransform);
	}
}