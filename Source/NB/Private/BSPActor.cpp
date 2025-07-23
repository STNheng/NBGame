// Fill out your copyright notice in the Description page of Project Settings.


#include "BSPActor.h"


// Sets default values
ABSPActor::ABSPActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootCom = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	//PCGComp = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG"));
	SetRootComponent(RootCom);
	
	
}

// Called when the game starts or when spawned
void ABSPActor::BeginPlay()
{
	Super::BeginPlay();
	StaticSectionIndex = 0;
	GenerateDungeon();
	RootCom->SetWorldLocation(FVector(1,1,0));
	//PCGComp->SetGraph(PCGGraph);
	//PCGComp->Refresh();
}

// Called every frame
void ABSPActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	for(auto Node : LeafNodes)
	{
		DrawDebugBox(
			GetWorld(),
			Node->Bounds.GetCenter(),
			Node->Bounds.GetExtent(),
			FColor::Green,
			false, 
			-1.f,  
			0,     
			5.f    
		);
	}
}

void ABSPActor::SplitNode(TSharedPtr<FBSPNode> Node, int32 Depth, int32 RoomSize)
{
	if (Depth <= 0 || Node->Bounds.GetSize().X < RoomSize * 2 || Node->Bounds.GetSize().Y < RoomSize * 2)
		return;

	FVector Size = Node->Bounds.GetSize();

	// 优先分割长边
	bool bSplitVertically = Size.X > Size.Y;

	if (bSplitVertically)
	{
		// X方向分割
		float Center = Size.X * 0.5f;
		float Offset = Size.X * 0.1f; // 允许±10%偏移
		float MinSplit = FMath::Clamp(Center - Offset, (float)RoomSize, Size.X - RoomSize);
		float MaxSplit = FMath::Clamp(Center + Offset, (float)RoomSize, Size.X - RoomSize);
		float Split = FMath::FRandRange(MinSplit, MaxSplit);

		FBox LeftBox(Node->Bounds.Min, Node->Bounds.Min + FVector(Split, Size.Y, Size.Z));
		FBox RightBox(Node->Bounds.Min + FVector(Split, 0, 0), Node->Bounds.Max);
		Node->Left = MakeShared<FBSPNode>(FBSPNode{ LeftBox });
		Node->Right = MakeShared<FBSPNode>(FBSPNode{ RightBox });
	}
	else
	{
		// Y方向分割
		float Center = Size.Y * 0.5f;
		float Offset = Size.Y * 0.1f;
		float MinSplit = FMath::Clamp(Center - Offset, (float)RoomSize, Size.Y - RoomSize);
		float MaxSplit = FMath::Clamp(Center + Offset, (float)RoomSize, Size.Y - RoomSize);
		float Split = FMath::FRandRange(MinSplit, MaxSplit);

		FBox LeftBox(Node->Bounds.Min, Node->Bounds.Min + FVector(Size.X, Split, Size.Z));
		FBox RightBox(Node->Bounds.Min + FVector(0, Split, 0), Node->Bounds.Max);
		Node->Left = MakeShared<FBSPNode>(FBSPNode{ LeftBox });
		Node->Right = MakeShared<FBSPNode>(FBSPNode{ RightBox });
	}

	SplitNode(Node->Left, Depth - 1, RoomSize);
	SplitNode(Node->Right, Depth - 1, RoomSize);
}

void ABSPActor::CreateCorridorBox(TSharedPtr<FBSPNode> Node)
{
	if (Node->IsLeafNode())
	{
		return;
	}
	//
	CreateCorridorBox(Node->Left);
	CreateCorridorBox(Node->Right);
	
	FVector LeftCenter, RightCenter;
	FBox LeftBox, RightBox;
	
	FindClosestRooms(Node->Left, Node->Right, LeftCenter, LeftBox, RightCenter, RightBox);
	
	bool bXAxis = FMath::Abs(LeftCenter.X - RightCenter.X) > FMath::Abs(LeftCenter.Y - RightCenter.Y);
	
	float CorridorWidth = 100.0f;
	float CorridorHeight = 10.0f;
	float CorridorCenter = 0.0f;
	float OverlapWidth = CorridorWidth;
	
	GetOverlapCenterAndWidth(LeftBox, RightBox, bXAxis, CorridorCenter, OverlapWidth);
	UE_LOG(LogTemp,Display,TEXT("LeftBox: %s   RightBox: %s  OverlapWidth: %f"),*LeftBox.GetCenter().ToString(),*RightBox.GetCenter().ToString(),OverlapWidth);
	FVector Start = GetEdgePoint(LeftBox, RightCenter, bXAxis);
	FVector End   = GetEdgePoint(RightBox, LeftCenter, bXAxis);
	//OverlapWidth = FMath::Min(OverlapWidth,200.f);
	if (bXAxis)
	{
		Node->Room.CorridorDirection = FVector(1, 0, 0);
		float MinX = FMath::Min(Start.X, End.X);
		float MaxX = FMath::Max(Start.X, End.X);
		Node->Room.CorridorBox = FBox(
			FVector(MinX, CorridorCenter - OverlapWidth * 0.3f, Start.Z),
			FVector(MaxX, CorridorCenter + OverlapWidth * 0.3f, Start.Z + CorridorHeight)
		);
		
	}
	else
	{
		Node->Room.CorridorDirection = FVector(0, 1, 0);
		float MinY = FMath::Min(Start.Y, End.Y);
		float MaxY = FMath::Max(Start.Y, End.Y);
		Node->Room.CorridorBox = FBox(
			FVector(CorridorCenter - OverlapWidth * 0.3f, MinY, Start.Z),
			FVector(CorridorCenter + OverlapWidth * 0.3f, MaxY, Start.Z + CorridorHeight)
		);
	}
	CorridorAndRoom Temp = {Node->Room.CorridorBox,LeftBox,RightBox};
	SpawnedCorridors.Add(Temp);
	
}

void ABSPActor::GenerateDungeon()
{
	// 1. 创建根节点
	LeafNodes.Empty();
	Root = MakeShared<FBSPNode>(FBSPNode{ FBox(FVector::ZeroVector, DungeonSize) });
	// 2. 分割空间
	SplitNode(Root, MaxDepth, MinRoomSize);
	// 4.获取所有的叶子节点方便后续生成房间
	GetLeafNodes(Root,LeafNodes);
	//用ProceduralMeshCom为每一个叶子结点生成平面，这个平面是房间的地板Floor
	GeneratePlanesForLeafNodes();
	UE_LOG(LogTemp,Display,TEXT("------%d------"),StaticSectionIndex);//8
	//生成走廊的盒体  
	CreateCorridorBox(Root);
	//为房间生成墙壁
	//GenerateWallsForLeafNodes();
	TArray<FBox> Corridors;
	GetCorridorBoxes(Root, Corridors);
	//生成走廊MEsh
	for (int32 i = 0; i < Corridors.Num(); ++i)
	{
		CreateBoxMesh(Corridors[i], StaticSectionIndex++);
	}
	//生成走廊墙壁
	CreateCorridorWall(StaticSectionIndex);
	CreateSplineForBounds();
}

void ABSPActor::GetLeafNodes(TSharedPtr<FBSPNode> Node, TArray<TSharedPtr<FBSPNode>>& OutLeaves)
{
	if (!Node) return;
	if (Node->IsLeafNode())
	{
		OutLeaves.Add(Node);
	}
	else
	{
		GetLeafNodes(Node->Left, OutLeaves);
		GetLeafNodes(Node->Right, OutLeaves);
	}
}

void ABSPActor::CreatePlaneInBox(const FBox& Box, int Idx)
{
	// 获取Box的尺寸和中心
	FVector BoxSize = Box.GetSize();
	FVector BoxCenter = Box.GetCenter();
    
	// 创建平面顶点（XY平面）
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FVector> Normals;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
    
	// 计算平面的四个顶点
	FVector HalfSize = BoxSize * FMath::RandRange(0.3,0.45);
	LeafNodes[Idx]->Room.RoomBounds = FBox(BoxCenter + FVector(-HalfSize.X, -HalfSize.Y, 0),BoxCenter + FVector( HalfSize.X,  HalfSize.Y, 0));
	// 按逆时针顺序添加顶点（从上方看）
	Vertices.Add(BoxCenter + FVector(-HalfSize.X, -HalfSize.Y, 0)); // 左下 (0)
	Vertices.Add(BoxCenter + FVector( HalfSize.X, -HalfSize.Y, 0)); // 右下 (1)
	Vertices.Add(BoxCenter + FVector( HalfSize.X,  HalfSize.Y, 0)); // 右上 (2)
	Vertices.Add(BoxCenter + FVector(-HalfSize.X,  HalfSize.Y, 0)); // 左上 (3)
    
	// 创建三角形（逆时针绕序，确保面朝上）
	Triangles.Add(0); Triangles.Add(2); Triangles.Add(1); // 第一个三角形
	Triangles.Add(0); Triangles.Add(3); Triangles.Add(2); // 第二个三角形
    
	// 创建平面网格
	ProceduralMesh->CreateMeshSection(Idx, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	ProceduralMesh->SetMaterial(Idx,MIns);
}

void ABSPActor::GeneratePlanesForLeafNodes()
{
	for (int32 i = 0; i < LeafNodes.Num(); i++)
	{
		CreatePlaneInBox(LeafNodes[i]->Bounds, StaticSectionIndex++);
	}
}

void ABSPActor::GetCorridorBoxes(TSharedPtr<FBSPNode> Node, TArray<FBox>& OutCorridors)
{
	if (!Node) return;
	if (!Node->IsLeafNode() && Node->Room.CorridorBox.GetVolume() > 0)
	{
		OutCorridors.Add(Node->Room.CorridorBox);
	}
	if (Node->Left) GetCorridorBoxes(Node->Left, OutCorridors);
	if (Node->Right) GetCorridorBoxes(Node->Right, OutCorridors);
}

void ABSPActor::CreateBoxMesh(const FBox& Box, int Idx)
{
	FVector Center = Box.GetCenter();
	FVector Extent = Box.GetExtent();

	// 创建平面顶点（XY平面，Z为Box底部）
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FVector> Normals;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// 计算平面的四个顶点（按逆时针顺序）
	Vertices.Add(Center + FVector(-Extent.X, -Extent.Y, 0)); // 左下 (0)
	Vertices.Add(Center + FVector( Extent.X, -Extent.Y, 0)); // 右下 (1)
	Vertices.Add(Center + FVector( Extent.X,  Extent.Y, 0)); // 右上 (2)
	Vertices.Add(Center + FVector(-Extent.X,  Extent.Y, 0)); // 左上 (3)

	// 创建三角形（逆时针绕序，确保面朝上）
	Triangles.Add(0); Triangles.Add(2); Triangles.Add(1); // 第一个三角形
	Triangles.Add(0); Triangles.Add(3); Triangles.Add(2); // 第二个三角形

	ProceduralMesh->CreateMeshSection(Idx, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	if (MIns)
	{
		ProceduralMesh->SetMaterial(Idx, MIns);
	}
}

FVector ABSPActor::GetEdgePoint(const FBox& RoomBox, const FVector& To, bool bXAxis)
{
	FVector Center = RoomBox.GetCenter();
	FVector Extent = RoomBox.GetExtent();
	FVector Result = Center;

	if (bXAxis)
	{
		// X方向：判断To在左还是右
		if (To.X > Center.X)
			Result.X += Extent.X; // 右边缘
		else
			Result.X -= Extent.X; // 左边缘
	}
	else
	{
		// Y方向：判断To在上还是下
		if (To.Y > Center.Y)
			Result.Y += Extent.Y; // 上边缘
		else
			Result.Y -= Extent.Y; // 下边缘
	}
	return Result;
}

FVector ABSPActor::FindClosestRoomCenter(TSharedPtr<FBSPNode> Node, const FVector& Target)
{
	if (!Node) return FVector::ZeroVector;
	if (Node->IsLeafNode())
		return Node->Room.RoomBounds.GetCenter();

	// 递归找左右子树中距离Target最近的房间中心
	FVector LeftCenter = FindClosestRoomCenter(Node->Left, Target);
	FVector RightCenter = FindClosestRoomCenter(Node->Right, Target);

	if ((LeftCenter - Target).SizeSquared() < (RightCenter - Target).SizeSquared())
	{
		return LeftCenter;
	}
	return RightCenter;
}

FBox ABSPActor::FindRoomBoundsByCenter(TSharedPtr<FBSPNode> Node, const FVector& Center)
{
	if (!Node) return FBox();
	if (Node->IsLeafNode())
		return Node->Room.RoomBounds;

	FBox LeftBox = FindRoomBoundsByCenter(Node->Left, Center);
	if (LeftBox.IsValid && LeftBox.GetCenter().Equals(Center, 0.1f))
		return LeftBox;
	FBox RightBox = FindRoomBoundsByCenter(Node->Right, Center);
	if (RightBox.IsValid && RightBox.GetCenter().Equals(Center, 0.1f))
		return RightBox;
	return FBox();
}

void ABSPActor::GetAllRoomCenters(TSharedPtr<FBSPNode> Node, TArray<FVector>& OutCenters, TArray<FBox>& OutRoomBounds)
{
	if (!Node) return;
	if (Node->IsLeafNode())
	{
		OutCenters.Add(Node->Room.RoomBounds.GetCenter());
		OutRoomBounds.Add(Node->Room.RoomBounds);
	}
	else
	{
		GetAllRoomCenters(Node->Left, OutCenters, OutRoomBounds);
		GetAllRoomCenters(Node->Right, OutCenters, OutRoomBounds);
	}
}

void ABSPActor::FindClosestRooms(TSharedPtr<FBSPNode> LeftNode, TSharedPtr<FBSPNode> RightNode, FVector& OutLeftCenter,
	FBox& OutLeftBox, FVector& OutRightCenter, FBox& OutRightBox)
{
	TArray<FVector> LeftCenters, RightCenters;
	TArray<FBox> LeftBoxes, RightBoxes;
	GetAllRoomCenters(LeftNode, LeftCenters, LeftBoxes);
	GetAllRoomCenters(RightNode, RightCenters, RightBoxes);

	float MinDistSq = TNumericLimits<float>::Max();
	int32 BestLeft = -1, BestRight = -1;
	for (int32 i = 0; i < LeftCenters.Num(); i++)
	{
		for (int32 j = 0; j < RightCenters.Num(); j++)
		{
			float CorridorCenter = 0.0f;
			float OverlapWidth = 0.0f;
			float DistSq = FVector::DistSquared(LeftCenters[i], RightCenters[j]);
			bool bXAxis = FMath::Abs(LeftCenters[i].X - RightCenters[j].X) > FMath::Abs(LeftCenters[i].Y - RightCenters[j].Y);
			GetOverlapCenterAndWidth(LeftBoxes[i], RightBoxes[j], bXAxis, CorridorCenter, OverlapWidth);
			//重叠部分超过80且距离更短。不然只有最短没有重叠会出bug
			if (DistSq < MinDistSq && OverlapWidth > 80.0f)
			{
				MinDistSq = DistSq;
				BestLeft = i;
				BestRight = j;
			}
		}
	}
	if (BestLeft >= 0 && BestRight >= 0)
	{
		OutLeftCenter = LeftCenters[BestLeft];
		OutLeftBox = LeftBoxes[BestLeft];
		OutRightCenter = RightCenters[BestRight];
		OutRightBox = RightBoxes[BestRight];
	}
}

void ABSPActor::GetOverlapCenterAndWidth(FBox& BoxA, FBox& BoxB, bool bXAxis, float& OutCenter,
	float& OutWidth)
{
	if (bXAxis)
	{
		// Y方向重叠
		float OverlapMin = FMath::Max(BoxA.Min.Y, BoxB.Min.Y);
		float OverlapMax = FMath::Min(BoxA.Max.Y, BoxB.Max.Y);
		if (OverlapMin < OverlapMax)
		{
			OutCenter = (OverlapMin + OverlapMax) * 0.5f;
			OutWidth = OverlapMax - OverlapMin;
		}
		else
		{
			// 没有重叠
			OutCenter = (BoxA.GetCenter().Y + BoxB.GetCenter().Y) * 0.5f;
			OutWidth = 1.0f;
		}
	}
	else
	{
		// X方向重叠
		float OverlapMin = FMath::Max(BoxA.Min.X, BoxB.Min.X);
		float OverlapMax = FMath::Min(BoxA.Max.X, BoxB.Max.X);
		if (OverlapMin < OverlapMax)
		{
			OutCenter = (OverlapMin + OverlapMax) * 0.5f;
			OutWidth = OverlapMax - OverlapMin;
		}
		else
		{
			OutCenter = (BoxA.GetCenter().X + BoxB.GetCenter().X) * 0.5f;
			OutWidth = 1.0f;
		}
	}
}

void ABSPActor::CreateWallMesh(const FBox& RoomBox, int32 SectionIndex, bool bIsFront, bool bIsBack, bool bIsLeft,
	bool bIsRight)
{
    float WallHeight = 200; // 墙高度
	
    TArray<FVector2D> UVs;
    TArray<FVector> Normals;
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;
	TArray<FVector> Points;
	FVector WallP0, WallP1;
	if (bIsFront)      GetWallEdgePoints(RoomBox, 0, WallP0, WallP1);
	else if (bIsBack)  GetWallEdgePoints(RoomBox, 1, WallP0, WallP1);
	else if (bIsLeft)  GetWallEdgePoints(RoomBox, 2, WallP0, WallP1);
	else if (bIsRight) GetWallEdgePoints(RoomBox, 3, WallP0, WallP1);
	Points.Add(WallP0);Points.Add(WallP1);
	TArray<FBox> AdjacentCorridors;
	//UE_LOG(LogTemp,Display,TEXT("WallP0: %s, WallP1: %s"),*WallP0.ToString(),*WallP1.ToString())
	for (const auto& tmp : SpawnedCorridors)
	{
		if (bIsRight && (IsRightWallAdjacentToCorridor(RoomBox, tmp.Corr)))
		{
			AdjacentCorridors.Add(tmp.Corr);
			GetWallEdgePoints(tmp.Corr,2,WallP0,WallP1);
			Points.Add(WallP0);Points.Add(WallP1);
			//UE_LOG(LogTemp,Display,TEXT("WallP0: %s, WallP1: %s"),*WallP0.ToString(),*WallP1.ToString())
		}
		else if (bIsLeft && (IsLeftWallAdjacentToCorridor(RoomBox, tmp.Corr)))
		{
			AdjacentCorridors.Add(tmp.Corr);
			GetWallEdgePoints(tmp.Corr,3,WallP0,WallP1);
			Points.Add(WallP0);Points.Add(WallP1);
			//UE_LOG(LogTemp,Display,TEXT("WallP0: %s, WallP1: %s"),*WallP0.ToString(),*WallP1.ToString())
		}
		else if (bIsFront && (IsFrontWallAdjacentToCorridor(RoomBox, tmp.Corr)))
		{
			AdjacentCorridors.Add(tmp.Corr);
			GetWallEdgePoints(tmp.Corr,1,WallP0,WallP1);
			Points.Add(WallP0);Points.Add(WallP1);
			//UE_LOG(LogTemp,Display,TEXT("WallP0: %s, WallP1: %s"),*WallP0.ToString(),*WallP1.ToString())
		}
		else if (bIsBack && (IsBackWallAdjacentToCorridor(RoomBox, tmp.Corr)))
		{
			AdjacentCorridors.Add(tmp.Corr);
			GetWallEdgePoints(tmp.Corr,0,WallP0,WallP1);
			Points.Add(WallP0);Points.Add(WallP1);
			//UE_LOG(LogTemp,Display,TEXT("WallP0: %s, WallP1: %s"),*WallP0.ToString(),*WallP1.ToString())
		}
	}
	//UE_LOG(LogTemp,Display,TEXT("-----------------------------------------------"))
	if (bIsFront || bIsBack)
	{
		Points.Sort([](const FVector& A, const FVector& B) {
			return A.X < B.X;
		});
	}
	else if (bIsLeft || bIsRight)
	{
		Points.Sort([](const FVector& A, const FVector& B) {
			return A.Y < B.Y;
		});
	}
	if(AdjacentCorridors.Num() == 0)
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		FVector Min = RoomBox.Min;
		FVector Max = RoomBox.Max;
		
		if (bIsFront) // 前墙 (Y = Min.Y)
		{
			Vertices.Add(FVector(Min.X, Min.Y, 0));
			Vertices.Add(FVector(Max.X, Min.Y, 0));
			Vertices.Add(FVector(Max.X, Min.Y, 200));
			Vertices.Add(FVector(Min.X, Min.Y, 200));
		}
		else if (bIsBack) // 后墙 (Y = Max.Y)
		{
			Vertices.Add(FVector(Min.X, Max.Y, 0));
			Vertices.Add(FVector(Max.X, Max.Y, 0));
			Vertices.Add(FVector(Max.X, Max.Y, 200));
			Vertices.Add(FVector(Min.X, Max.Y, 200));
		}
		else if (bIsLeft) // 左墙 (X = Min.X)
		{
			Vertices.Add(FVector(Min.X, Min.Y, 0));
			Vertices.Add(FVector(Min.X, Max.Y, 0));
			Vertices.Add(FVector(Min.X, Max.Y, 200));
			Vertices.Add(FVector(Min.X, Min.Y, 200));
		}
		else if (bIsRight) // 右墙 (X = Max.X)
		{
			Vertices.Add(FVector(Max.X, Min.Y, 0));
			Vertices.Add(FVector(Max.X, Max.Y, 0));
			Vertices.Add(FVector(Max.X, Max.Y, 200));
			Vertices.Add(FVector(Max.X, Min.Y, 200));
		}
 
		// 三角形
		Triangles = {0, 1, 2, 0, 2, 3};
 
		// // UV坐标
		//UVs = {FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)};
		// 法线（指向房间内部）
		FVector Normal;
		if (bIsFront) Normal = FVector(0, 1, 0);
		else if (bIsBack) Normal = FVector(0, -1, 0);
		else if (bIsLeft) Normal = FVector(1, 0, 0);
		else if (bIsRight) Normal = FVector(-1, 0, 0);
 	//
		Normals.Init(Normal, 4);
		// VertexColors.Init(FColor::White, 4);
		// Tangents.Init(FProcMeshTangent(1, 0, 0), 4);
 
		ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
		if (MIns) ProceduralMesh->SetMaterial(SectionIndex, MIns);
	}
	else
	{
	    for(int i = 0;i<Points.Num();i++)
	    {
			UE_LOG(LogTemp, Log, TEXT("Poins: %s"), *Points[i].ToString());
	    }
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		for(int i = 0;i<Points.Num();i= i + 2)
		{
			//CreateMEsh(Points[i],Points[i + 1],bIsFront,bIsBack,bIsLeft,bIsRight,SectionIndex++);
			FVector Min = Points[i];
			FVector Max = Points[i + 1];
			if (bIsFront) // 前墙 (Y = Min.Y)
			{
				Vertices.Add(FVector(Min.X, Min.Y, 0));
				Vertices.Add(FVector(Max.X, Min.Y, 0));
				Vertices.Add(FVector(Max.X, Min.Y, 200));
				Vertices.Add(FVector(Min.X, Min.Y, 200));
			}
			else if (bIsBack) // 后墙 (Y = Max.Y)
			{
				Vertices.Add(FVector(Min.X, Max.Y, 0));
				Vertices.Add(FVector(Max.X, Max.Y, 0));
				Vertices.Add(FVector(Max.X, Max.Y, 200));
				Vertices.Add(FVector(Min.X, Max.Y, 200));
			}
			else if (bIsLeft) // 左墙 (X = Min.X)
			{
				Vertices.Add(FVector(Min.X, Min.Y, 0));
				Vertices.Add(FVector(Min.X, Max.Y, 0));
				Vertices.Add(FVector(Min.X, Max.Y, 200));
				Vertices.Add(FVector(Min.X, Min.Y, 200));
			}
			else if (bIsRight) // 右墙 (X = Max.X)
			{
				Vertices.Add(FVector(Max.X, Min.Y, 0));
				Vertices.Add(FVector(Max.X, Max.Y, 0));
				Vertices.Add(FVector(Max.X, Max.Y, 200));
				Vertices.Add(FVector(Max.X, Min.Y, 200));
			}
			// for(int j = 0;j<4;j++)
			// {
			// 	UE_LOG(LogTemp, Log, TEXT("Vertices: %s"), *Vertices[j].ToString());
			// }
			// 三角形
			//Triangles = {0, 2, 1, 0, 3, 2}
			//Triangles.Append({2*i, 2*i + 1, 2*i + 2, 2*i, 2*i + 2, 2*i + 3});
			if (bIsRight || bIsFront)
			{
				
				Triangles.Append({2*i, 2*i + 1, 2*i + 2, 2*i, 2*i + 2, 2*i + 3});
			}
			else
			{
				Triangles.Append({2*i, 2*i + 2, 2*i + 1, 2*i, 2*i + 3, 2*i + 2});
			}
			
		}
		ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
		if (MIns) ProceduralMesh->SetMaterial(SectionIndex, MIns);
	}
    
}

void ABSPActor::GenerateWallsForLeafNodes()
{
	for (int32 i = 0; i < LeafNodes.Num(); i++)
	{
		const FBox& RoomBox = LeafNodes[i]->Room.RoomBounds;
        
		// 生成4个墙面
		CreateWallMesh(RoomBox, StaticSectionIndex++, true, false, false, false);  // 前墙
		CreateWallMesh(RoomBox, StaticSectionIndex++, false, true, false, false);  // 后墙
		CreateWallMesh(RoomBox, StaticSectionIndex++, false, false, true, false);  // 左墙
		CreateWallMesh(RoomBox, StaticSectionIndex++, false, false, false, true);  // 右墙
	}
}

bool ABSPActor::IsBoxesOverlapping(const FBox& BoxA, const FBox& BoxB)
{
	return (BoxA.Min.X <= BoxB.Max.X && BoxA.Max.X >= BoxB.Min.X) &&
		   (BoxA.Min.Y <= BoxB.Max.Y && BoxA.Max.Y >= BoxB.Min.Y) &&
		   (BoxA.Min.Z <= BoxB.Max.Z && BoxA.Max.Z >= BoxB.Min.Z);
}

bool ABSPActor::IsRightWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox)
{
	// 右墙X与走廊左边X重合
	if (FMath::IsNearlyEqual(RoomBox.Max.X, CorridorBox.Min.X, 1.0f))
	{
		// Y区间有重叠
		return (RoomBox.Min.Y < CorridorBox.Max.Y && RoomBox.Max.Y > CorridorBox.Min.Y);
	}
	return false;
}

bool ABSPActor::IsFrontWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox)
{
	if (FMath::IsNearlyEqual(RoomBox.Min.Y, CorridorBox.Max.Y, 1.0f))
	{
		return (RoomBox.Min.X < CorridorBox.Max.X && RoomBox.Max.X > CorridorBox.Min.X);
	}
	return false;
}

bool ABSPActor::IsLeftWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox)
{
	if (FMath::IsNearlyEqual(RoomBox.Min.X, CorridorBox.Max.X, 1.0f))
	{
		return (RoomBox.Min.Y < CorridorBox.Max.Y && RoomBox.Max.Y > CorridorBox.Min.Y);
	}
	return false;
}

bool ABSPActor::IsBackWallAdjacentToCorridor(const FBox& RoomBox, const FBox& CorridorBox)
{
	if (FMath::IsNearlyEqual(RoomBox.Max.Y, CorridorBox.Min.Y, 1.0f))
	{
		return (RoomBox.Min.X < CorridorBox.Max.X && RoomBox.Max.X > CorridorBox.Min.X);
	}
	return false;
}

void ABSPActor::GetWallEdgePoints(const FBox& Box, int direction, FVector& OutP0, FVector& OutP1)
{
	FVector Min = Box.Min;
	FVector Max = Box.Max;
	UE_LOG(LogTemp,Display,TEXT("direction:%d"),direction)
	switch (direction)
	{
	case 0: // Front (Y = Min.Y)
		OutP0 = FVector(Min.X, Min.Y, 0);
		OutP1 = FVector(Max.X, Min.Y, 0);
		break;
	case 1: // Back (Y = Max.Y)
		OutP0 = FVector(Min.X, Max.Y, 0);
		OutP1 = FVector(Max.X, Max.Y, 0);
		break;
	case 2: // Left (X = Min.X)
		OutP0 = FVector(Min.X, Min.Y, 0);
		OutP1 = FVector(Min.X, Max.Y, 0);
		break;
	case 3: // Right (X = Max.X)
		OutP0 = FVector(Max.X, Min.Y, 0);
		OutP1 = FVector(Max.X, Max.Y, 0);
		break;
	default:break;
	}
}

void ABSPActor::CreateMEsh(FVector Min, FVector Max, bool bIsFront, bool bIsBack, bool bIsLeft, bool bIsRight,int SectionIndex)
{
	TArray<FVector2D> UVs;
	TArray<FVector> Normals;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	// FVector Min = Points[i];
	// FVector Max = Points[i + 1];
	if (bIsFront) // 前墙 (Y = Min.Y)
	{
		Vertices.Add(FVector(Min.X, Min.Y, 0));
		Vertices.Add(FVector(Max.X, Min.Y, 0));
		Vertices.Add(FVector(Max.X, Min.Y, 200));
		Vertices.Add(FVector(Min.X, Min.Y, 200));
	}
	else if (bIsBack) // 后墙 (Y = Max.Y)
	{
		Vertices.Add(FVector(Min.X, Max.Y, 0));
		Vertices.Add(FVector(Max.X, Max.Y, 0));
		Vertices.Add(FVector(Max.X, Max.Y, 200));
		Vertices.Add(FVector(Min.X, Max.Y, 200));
	}
	else if (bIsLeft) // 左墙 (X = Min.X)
	{
		Vertices.Add(FVector(Min.X, Min.Y, 0));
		Vertices.Add(FVector(Min.X, Max.Y, 0));
		Vertices.Add(FVector(Min.X, Max.Y, 200));
		Vertices.Add(FVector(Min.X, Min.Y, 200));
	}
	else if (bIsRight) // 右墙 (X = Max.X)
	{
		Vertices.Add(FVector(Max.X, Min.Y, 0));
		Vertices.Add(FVector(Max.X, Max.Y, 0));
		Vertices.Add(FVector(Max.X, Max.Y, 200));
		Vertices.Add(FVector(Max.X, Min.Y, 200));
	}
	for(int j = 0;j<4;j++)
	{
		UE_LOG(LogTemp, Log, TEXT("Vertices: %s"), *Vertices[j].ToString());
	}
	// 三角形
	Triangles = {0, 1, 2, 0, 2, 3};
	ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	if (MIns)
	{
		ProceduralMesh->SetMaterial(SectionIndex, MIns);
	}
	
}

void ABSPActor::CreateCorridorWall(int32 SectionIndex)
{
	for(auto tmp : SpawnedCorridors)
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector2D> UVs;
		TArray<FVector> Normals;
		TArray<FColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;
		FVector WallP0,WallP1,WallP2,WallP3;
		//从debug信息上来看，只有0,3两种情况。
		if(IsRightWallAdjacentToCorridor(tmp.RoomA,tmp.Corr))
		{
			GetWallEdgePoints(tmp.Corr,0,WallP0,WallP1);
			GetWallEdgePoints(tmp.Corr,1,WallP2,WallP3);
			UE_LOG(LogTemp,Display,TEXT("Direction: 0   WallP0: %s   WallP1: %s WallP2: %s   WallP3: %s"),*WallP0.ToString(),*WallP1.ToString(),*WallP2.ToString(),*WallP3.ToString())
			Triangles = {0,1,2,0,2,3,4,6,5,4,7,6};
		}
		else if (IsLeftWallAdjacentToCorridor(tmp.RoomA,tmp.Corr))
		{
			GetWallEdgePoints(tmp.Corr,0,WallP0,WallP1);
			GetWallEdgePoints(tmp.Corr,1,WallP2,WallP3);
			UE_LOG(LogTemp,Display,TEXT("Direction: 1   WallP0: %s   WallP1: %s WallP2: %s   WallP3: %s"),*WallP0.ToString(),*WallP1.ToString(),*WallP2.ToString(),*WallP3.ToString())
			Triangles = {0,1,2,0,2,3,4,6,5,4,7,6};
		}
		else if (IsFrontWallAdjacentToCorridor(tmp.RoomA,tmp.Corr))
		{
			GetWallEdgePoints(tmp.Corr,2,WallP0,WallP1);
			GetWallEdgePoints(tmp.Corr,3,WallP2,WallP3);
			UE_LOG(LogTemp,Display,TEXT("Direction: 2   WallP0: %s   WallP1: %s WallP2: %s   WallP3: %s"),*WallP0.ToString(),*WallP1.ToString(),*WallP2.ToString(),*WallP3.ToString())
			Triangles = {0,2,1,0,3,2,4,5,6,4,6,7};
		}
		else if (IsBackWallAdjacentToCorridor(tmp.RoomA,tmp.Corr))
		{
			GetWallEdgePoints(tmp.Corr,2,WallP0,WallP1);
			GetWallEdgePoints(tmp.Corr,3,WallP2,WallP3);
			UE_LOG(LogTemp,Display,TEXT("Direction: 3   WallP0: %s   WallP1: %s WallP2: %s   WallP3: %s"),*WallP0.ToString(),*WallP1.ToString(),*WallP2.ToString(),*WallP3.ToString())
			Triangles = {0,2,1,0,3,2,4,5,6,4,6,7};
		}
		
		Vertices.Add(WallP0);
		Vertices.Add(WallP1);
		Vertices.Add(FVector(WallP1.X,WallP1.Y,200));
		Vertices.Add(FVector(WallP0.X,WallP0.Y,200));
		Vertices.Add(WallP2);
		Vertices.Add(WallP3);
		Vertices.Add(FVector(WallP3.X,WallP3.Y,200));
		Vertices.Add(FVector(WallP2.X,WallP2.Y,200));
		
		ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
		if (MIns)
		{
			ProceduralMesh->SetMaterial(SectionIndex++, MIns);
		}
	}
	
}

void ABSPActor::CreateSplineForBounds()
{
	for (int i = 0; i < LeafNodes.Num(); i++)
	{
		BoundsSpline.Add(NewObject<USplineComponent>(this, *FString::Printf(TEXT("Spline%d"),i)));
		BoundsSpline[i]->SetClosedLoop(true);
		BoundsSpline[i]->RegisterComponent();
		
		FVector P0,P1,P2,P3;
		GetWallEdgePoints(LeafNodes[i]->Room.RoomBounds,0,P0,P1);
		GetWallEdgePoints(LeafNodes[i]->Room.RoomBounds,1,P2,P3);
		BoundsSpline[i]->SetWorldLocation(P0);
		BoundsSpline[i]->AddSplinePoint(P0, ESplineCoordinateSpace::World);
		BoundsSpline[i]->AddSplinePoint(P1, ESplineCoordinateSpace::World);
		BoundsSpline[i]->AddSplinePoint(P3, ESplineCoordinateSpace::World);
		BoundsSpline[i]->AddSplinePoint(P2, ESplineCoordinateSpace::World);
		BoundsSpline[i]->SetSplinePointType(0, ESplinePointType::Linear, true);
		BoundsSpline[i]->SetSplinePointType(1, ESplinePointType::Linear, true);
		BoundsSpline[i]->SetSplinePointType(2, ESplinePointType::Linear, true);
		//BoundsSpline[i]->SetSplinePointType(3, ESplinePointType::Linear, true);
	}
	
	//Spline->RegisterComponent();
	
	
	//TArray<FVector> Corners = GetWallEdgePoints(Box, Box.Min.Z); // 或Box.Max.Z
	// for (int32 i = 0; i < Corners.Num(); ++i)
	// {
	// 	Spline->AddSplinePoint(Corners[i], ESplineCoordinateSpace::World);
	// }
	// Spline->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}



