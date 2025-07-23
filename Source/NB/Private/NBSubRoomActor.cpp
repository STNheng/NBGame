// Fill out your copyright notice in the Description page of Project Settings.


#include "NB/Public/NBSubRoomActor.h"

// Sets default values
ANBSubRoomActor::ANBSubRoomActor()
{
 	
	PrimaryActorTick.bCanEverTick = false;
	//Create and set up Component's Attachment
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Wall_1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_1"));
	Wall_2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_2"));
	Wall_3 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_3"));
	Wall_4 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_4"));
	Wall_5 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_5"));
	Wall_6 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_6"));
	Wall_7 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_7"));
	Wall_8 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall_8"));
	Floor->SetupAttachment(RootComponent);
	Wall_1->SetupAttachment(RootComponent);
	Wall_2->SetupAttachment(RootComponent);
	Wall_3->SetupAttachment(RootComponent);
	Wall_4->SetupAttachment(RootComponent);
	Wall_5->SetupAttachment(RootComponent);
	Wall_6->SetupAttachment(RootComponent);
	Wall_7->SetupAttachment(RootComponent);
	Wall_8->SetupAttachment(RootComponent);
	//set Floor's length and width
	Floor->SetRelativeScale3D(FVector3d(Length,Width,1.0));
	Wall_1->SetRelativeScale3D(FVector3d(1.0,Width,10));
	Wall_2->SetRelativeScale3D(FVector3d(1.0,Width,10));
	Wall_3->SetRelativeScale3D(FVector3d(Length,1.0,10));
	Wall_4->SetRelativeScale3D(FVector3d(Length,1.0,10));
	
	Wall_1->SetRelativeLocation(FVector3d(50 * Length,0,0));
	Wall_2->SetRelativeLocation(FVector3d(-50 * Length,0,0));
	Wall_3->SetRelativeLocation(FVector3d(0,50 * Width,0));
	Wall_4->SetRelativeLocation(FVector3d(0,-50 * Width,0));
	
}



// Called when the game starts or when spawned
void ANBSubRoomActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANBSubRoomActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ANBSubRoomActor::ReBuildRoom(int32 Len, int32 Wid)
{
	Floor->SetRelativeScale3D(FVector3d(Len,Wid,1.0));
	Wall_1->SetRelativeScale3D(FVector3d(1.0,Wid,10));
	Wall_2->SetRelativeScale3D(FVector3d(1.0,Wid,10));
	Wall_3->SetRelativeScale3D(FVector3d(Len,1.0,10));
	Wall_4->SetRelativeScale3D(FVector3d(Len,1.0,10));
	
	Wall_1->SetRelativeLocation(FVector3d(50 * Len,0,0));
	Wall_2->SetRelativeLocation(FVector3d(-50 * Len,0,0));
	Wall_3->SetRelativeLocation(FVector3d(0,50 * Wid,0));
	Wall_4->SetRelativeLocation(FVector3d(0,-50 * Wid,0));
	
}

