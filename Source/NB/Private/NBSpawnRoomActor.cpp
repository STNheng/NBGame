#include "NB/Public/NBSpawnRoomActor.h"


  // Sets default values
ANBSpawnRoomActor::ANBSpawnRoomActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ANBSpawnRoomActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANBSpawnRoomActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// bool ANBSpawnRoomActor::CanSpawnRoom(USceneComponent* Scene)
// {
// 	
// 	return false;
// }
//
// void ANBSpawnRoomActor::AddWorldLocation()
// {
// 	
// }

