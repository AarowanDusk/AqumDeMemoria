// Copyright, Bhargav Jt. Gogoi (Aarowan Vespera)


#include "Player/PlayerState/ADMPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"

AADMPlayerState::AADMPlayerState()
{

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UAttributeSet>("AttributeSet");

	NetUpdateFrequency = 100.f;

}

UAbilitySystemComponent* AADMPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
