#include "Components/BaseHealthComponent.h"

UBaseHealthComponent::UBaseHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaseHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(0.0f, MaxHealth);
	CurrentHealth = MaxHealth;
}

void UBaseHealthComponent::Treat(float HealAmount)
{
	if (HealAmount <= 0.0f || CurrentHealth >= MaxHealth)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	const float ActualHealAmount = CurrentHealth - OldHealth;

	if (ActualHealAmount > 0.0f)
	{
		BroadcastHealthChanged(OldHealth);
		OnHealed.Broadcast(ActualHealAmount, OldHealth, CurrentHealth);
	}
}

float UBaseHealthComponent::ApplyDamage(float DamageAmount, float& OutOldHealth, float& OutActualDamage)
{
	OutOldHealth = CurrentHealth;
	OutActualDamage = 0.0f;

	if (IsDead() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	OutActualDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Clamp(CurrentHealth - OutActualDamage, 0.0f, MaxHealth);

	if (OutActualDamage > 0.0f)
	{
		BroadcastHealthChanged(OutOldHealth);

		if (OutOldHealth > 0.0f && IsDead())
		{
			OnDeath.Broadcast();
		}
	}

	return OutActualDamage;
}

void UBaseHealthComponent::SetMaxHealth(float NewMaxHealth, bool bKeepHealthPercent)
{
	const float OldHealth = CurrentHealth;
	const float OldMaxHealth = MaxHealth;
	const float OldPercent = OldMaxHealth > 0.0f ? CurrentHealth / OldMaxHealth : 0.0f;

	MaxHealth = FMath::Max(0.0f, NewMaxHealth);
	CurrentHealth = bKeepHealthPercent
		? FMath::Clamp(MaxHealth * OldPercent, 0.0f, MaxHealth)
		: FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth) || !FMath::IsNearlyEqual(OldMaxHealth, MaxHealth))
	{
		BroadcastHealthChanged(OldHealth);

		if (OldHealth > 0.0f && IsDead())
		{
			OnDeath.Broadcast();
		}
	}
}

void UBaseHealthComponent::ResetHealth()
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = MaxHealth;
	bSuppressNextNonLethalHitReaction = false;

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		BroadcastHealthChanged(OldHealth);
	}
}

bool UBaseHealthComponent::IsDead() const
{
	return CurrentHealth <= 0.0f;
}

float UBaseHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UBaseHealthComponent::SuppressNextNonLethalHitReaction()
{
	bSuppressNextNonLethalHitReaction = true;
}

bool UBaseHealthComponent::ConsumeNextNonLethalHitReactionSuppressed()
{
	const bool bResult = bSuppressNextNonLethalHitReaction;
	bSuppressNextNonLethalHitReaction = false;
	return bResult;
}

void UBaseHealthComponent::BroadcastHealthChanged(float OldHealth)
{
	OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth);
}
