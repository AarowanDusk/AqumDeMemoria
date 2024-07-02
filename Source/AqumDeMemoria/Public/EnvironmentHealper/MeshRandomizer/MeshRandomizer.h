// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeshRandomizer.generated.h"

UCLASS()
class AQUMDEMEMORIA_API AMeshRandomizer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshRandomizer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
