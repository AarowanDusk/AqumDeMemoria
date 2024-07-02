// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)


#include "Player/PlayerController/ADMPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Characters/Player/ADMCharacter.h"

AADMPlayerController::AADMPlayerController()
{

	bReplicates = true;

}

void AADMPlayerController::BeginPlay()
{

	Super::BeginPlay();

	check(ADMContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(ADMContext, 0);
	}

	bShowMouseCursor = false;
	DefaultMouseCursor = EMouseCursor::Hand;

}

void AADMPlayerController::SetupInputComponent()
{

	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AADMPlayerController::Move);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AADMPlayerController::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AADMPlayerController::StopJumping);

	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AADMPlayerController::Look);

}

void AADMPlayerController::Move(const FInputActionValue& InputActionValue)
{

	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();

	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDrection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDrection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{

		ControlledPawn->AddMovementInput(ForwardDrection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDrection, InputAxisVector.X);

	}

}

void AADMPlayerController::Jump()
{

	if (GetCharacter())
	{
	
		GetCharacter()->Jump();

	}

}

void AADMPlayerController::StopJumping()
{

	if (GetCharacter())
	{

		GetCharacter()->StopJumping();

	}

}

void AADMPlayerController::Look(const FInputActionValue& InputActionValue)
{
	// input is a Vector2D
	FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{

		// add yaw and pitch input to controller
		ControlledPawn->AddControllerYawInput(LookAxisVector.X);
		ControlledPawn->AddControllerPitchInput(LookAxisVector.Y);

	}
}