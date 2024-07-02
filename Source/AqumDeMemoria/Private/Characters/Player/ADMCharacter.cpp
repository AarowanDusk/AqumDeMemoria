// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)


#include "Characters/Player/ADMCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CharacterMovementComponent/ADMCharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Player/PlayerState/ADMPlayerState.h"
#include "AbilitySystemComponent.h"

AADMCharacter::AADMCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UADMCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    ADMCharacterMovementComponent = Cast<UADMCharacterMovementComponent>(GetCharacterMovement());

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

    bReplicates = true;

}

void AADMCharacter::PossessedBy(AController* NewController)
{

    Super::PossessedBy(NewController);

    //init ability Actor info for the Server
    InitAbilityFunction();

}

void AADMCharacter::OnRep_PlayerState()
{

    Super::OnRep_PlayerState();

    //init ability Actor info for the Client
    InitAbilityFunction();

}

void AADMCharacter::Jump()
{

    Super::Jump();

    bADMPressedJump = true;

    bPressedJump = false;

}

void AADMCharacter::StopJumping()
{

    Super::StopJumping();

    bADMPressedJump = false;

}

FCollisionQueryParams AADMCharacter::GetIgnoreCharacterParams() const
{

    FCollisionQueryParams Params;

    TArray<AActor*> CharacterChildren;
    GetAllChildActors(CharacterChildren);
    Params.AddIgnoredActors(CharacterChildren);
    Params.AddIgnoredActor(this);

    return Params;

}

void AADMCharacter::Tick(float DeltaSeconds)
{

    Super::Tick(DeltaSeconds);

}

FAimOffset AADMCharacter::CalculateAimOffset()
{

    FRotator NormalizedDeltaRotate = UKismetMathLibrary::NormalizedDeltaRotator(this->GetBaseAimRotation(), this->GetActorRotation());

    Offsets.AimPitch = FMath::FInterpTo
    (
        Offsets.AimPitch,
        NormalizedDeltaRotate.Pitch,
        GetWorld()->DeltaTimeSeconds,
        10
    );

    float YawCalculation = (UKismetMathLibrary::Abs(NormalizedDeltaRotate.Yaw) > 130) ? 0 : NormalizedDeltaRotate.Yaw;

    Offsets.AimYaw = FMath::FInterpTo
    (
        Offsets.AimYaw,
        YawCalculation,
        GetWorld()->DeltaTimeSeconds,
        10
    );

    return Offsets;
}

float AADMCharacter::CalculateLean(float LeanSensitivity)
{

    FRotator CurrentRotaton = this->GetActorRotation();

    FRotator YawCalculation = UKismetMathLibrary::NormalizedDeltaRotator(PreviousRotation, CurrentRotaton);

    LeanAmount = FMath::FInterpTo
    (
        LeanAmount,
        YawCalculation.Yaw,
        GetWorld()->DeltaTimeSeconds,
        10
    );

    PreviousRotation = this->GetActorRotation();

    return LeanAmount;

}

float AADMCharacter::CalculateSpeed()
{

    return this->GetVelocity().Size();

}

void AADMCharacter::InitAbilityFunction()
{

    AADMPlayerState* ADMPlayerState = GetPlayerState<AADMPlayerState>();
    check(ADMPlayerState);

    ADMPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(ADMPlayerState, this);
    AbilitySystemComponent = ADMPlayerState->GetAbilitySystemComponent();
    AttributeSet = ADMPlayerState->GetAttributeSet();

}
