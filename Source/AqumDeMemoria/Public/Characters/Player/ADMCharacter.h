// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)

#pragma once

#include "CoreMinimal.h"
#include "Characters/Base/ADMCharacterBase.h"
#include "ADMCharacter.generated.h"

/**
 * 
 */

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UADMCharacterMovementComponent;

struct FInputActionValue;

USTRUCT(BlueprintType)
struct FAimOffset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float AimPitch;
	UPROPERTY(BlueprintReadOnly)
	float AimYaw;
};


UCLASS()
class AQUMDEMEMORIA_API AADMCharacter : public AADMCharacterBase
{
	GENERATED_BODY()

public:

	bool bADMPressedJump;

	AADMCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PossessedBy(AController* NewController) override;

	virtual void OnRep_PlayerState() override;

	virtual void Jump() override;

	virtual void StopJumping() override;

	FCollisionQueryParams GetIgnoreCharacterParams() const;

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE UADMCharacterMovementComponent* GetCharacterMovementComponent() const { return ADMCharacterMovementComponent; }

protected:

	virtual void Tick(float DeltaSeconds) override;

private:

	UPROPERTY(EditAnywhere, category = "Input")
	USpringArmComponent* CameraBoom;

	UPROPERTY(EditAnywhere, category = "Input")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, category = "Input")
	TObjectPtr<UADMCharacterMovementComponent> ADMCharacterMovementComponent;

	UPROPERTY()
	FRotator PreviousRotation = this->GetActorRotation();

	UPROPERTY()
	float LeanAmount = 0;

	UPROPERTY()
	FAimOffset Offsets;


	UFUNCTION(BlueprintCallable)
	FAimOffset CalculateAimOffset();

	UFUNCTION(BlueprintCallable)
	float CalculateLean(float LeanSensitivity);

	UFUNCTION(BlueprintCallable)
	float CalculateSpeed();

	void InitAbilityFunction();

};
