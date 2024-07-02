// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ADMPlayerController.generated.h"

/**
 * 
 */

class UInputMappingContext;
class UInputAction;

struct FInputActionValue;

UCLASS()
class AQUMDEMEMORIA_API AADMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AADMPlayerController();

protected:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

private:

	UPROPERTY(EditAnywhere, category = "Input")
	TObjectPtr<UInputMappingContext> ADMContext;

	UPROPERTY(EditAnywhere, category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UFUNCTION()
	void Move(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Jump();

	UFUNCTION()
	void StopJumping();

	UFUNCTION()
	void Look(const FInputActionValue& InputActionValue);


};
