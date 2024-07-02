// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Base/ADMCharacterBase.h"
#include "Components/CharacterMovementComponent/ADMCharacterMovementComponent.h"

// Sets default values
AADMCharacterBase::AADMCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UADMCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

UAbilitySystemComponent* AADMCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void AADMCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

