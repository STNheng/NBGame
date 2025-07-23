// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NBSubRoomActor.generated.h"

UCLASS()
class NB_API ANBSubRoomActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANBSubRoomActor();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Begin NB Game
public:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Floor;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_1;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_2;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_3;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_4;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_5;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_6;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_7;
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Wall_8;
	UPROPERTY(BlueprintReadWrite,EditAnywhere,meta=(ExposeOnSpawn))
	int32 Length = 10;
	UPROPERTY(BlueprintReadWrite,EditAnywhere,meta=(ExposeOnSpawn))
	int32 Width = 5;

	UFUNCTION(BlueprintCallable)
	void ReBuildRoom(int32 Len, int32 Wid);
	//End NB Game
	
	
};
