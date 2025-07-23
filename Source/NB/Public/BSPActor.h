// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "BSPActor.generated.h"



UCLASS()
class NB_API ABSPActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABSPActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinRoomSize = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxDepth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DungeonSize = FVector(2000, 2000, 400);

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)//地面和墙壁的材质
	UMaterialInterface* MIns;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)//Root
	USceneComponent* RootCom;
	
	struct FRoom
	{
		FBox RoomBounds;
		FBox CorridorBox;
		FVector CorridorDirection;
		TArray<FBox> ConnectedCorridors;
	};
	struct CorridorAndRoom
	{
		FBox Corr;
		FBox RoomA;
		FBox RoomB;
	};
	struct FBSPNode
	{
		FBox Bounds;
		TSharedPtr<FBSPNode> Left;
		TSharedPtr<FBSPNode> Right;
		FRoom Room;
		bool IsLeafNode(){ return Left == nullptr and Right == nullptr; }
	};
	
	TSharedPtr<FBSPNode> Root;
	TArray<TSharedPtr<FBSPNode>> LeafNodes;
	TArray<CorridorAndRoom> SpawnedCorridors;
	int32 StaticSectionIndex;
	
	void GenerateDungeon();
	void SplitNode(TSharedPtr<FBSPNode> Node, int32 Depth, int32 RoomSize);
	void CreateCorridorBox(TSharedPtr<FBSPNode> Node);
	
	void GetLeafNodes(TSharedPtr<FBSPNode> Node, TArray<TSharedPtr<FBSPNode>>& OutLeaves);
	
	void CreatePlaneInBox(const FBox& Box, int Idx);
	void GeneratePlanesForLeafNodes();
	
	void GetCorridorBoxes(TSharedPtr<FBSPNode> Node, TArray<FBox>& OutCorridors);
	void CreateBoxMesh(const FBox& Box, int Idx);
	FVector GetEdgePoint(const FBox& RoomBox, const FVector& To, bool bXAxis);
	
	FVector FindClosestRoomCenter(TSharedPtr<FBSPNode> Node, const FVector& Target);
	FBox FindRoomBoundsByCenter(TSharedPtr<FBSPNode> Node, const FVector& Center);
	void GetAllRoomCenters(TSharedPtr<FBSPNode> Node, TArray<FVector>& OutCenters, TArray<FBox>& OutRoomBounds);
	void FindClosestRooms(TSharedPtr<FBSPNode> LeftNode, TSharedPtr<FBSPNode> RightNode, FVector& OutLeftCenter, FBox& OutLeftBox, FVector& OutRightCenter, FBox& OutRightBox);
	void GetOverlapCenterAndWidth(FBox& BoxA, FBox& BoxB, bool bXAxis,float& OutCenter, float& OutWidth);
	void CreateWallMesh(const FBox& RoomBox, int32 SectionIndex, bool bIsFront, bool bIsBack, bool bIsLeft, bool bIsRight);
	void GenerateWallsForLeafNodes();
	
	bool IsBoxesOverlapping(const FBox& BoxA, const FBox& BoxB);
	bool IsRightWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox);
	bool IsFrontWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox);
	bool IsLeftWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox);
	bool IsBackWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox);
	// direction: 0=Front, 1=Back, 2=Left, 3=Right
	void GetWallEdgePoints(const FBox& Box, int direction, FVector& OutP0, FVector& OutP1);
	void CreateMEsh(FVector Min,FVector Max,bool bIsFront, bool bIsBack, bool bIsLeft, bool bIsRight,int SectionIndex);
	void CreateCorridorWall(int32 SectionIndex);
	UPROPERTY(VisibleAnywhere)
	TArray<USplineComponent*> BoundsSpline;
	void CreateSplineForBounds();
	// UPROPERTY(EditAnywhere,BlueprintReadWrite)
	// UPCGComponent* PCGComp;
	
};
