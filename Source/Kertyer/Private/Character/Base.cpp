#include "Character/Base.h"
#include "Components/BaseHealthComponent.h"
#include "Components/CombatComponent.h"
#include "Components/CombatReactionComponent.h"
#include "Components/DefenseComponent.h"
#include "Components/WeaponHolderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/WeaponBase.h"

namespace
{
	FString BaseSafeActorName(const AActor* Actor)
	{
		return IsValid(Actor) ? Actor->GetName() : TEXT("无");
	}
}

ABase::ABase()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseHealthComponent = CreateDefaultSubobject<UBaseHealthComponent>(TEXT("BaseHealthComponent"));
	WeaponHolderComponent = CreateDefaultSubobject<UWeaponHolderComponent>(TEXT("WeaponHolderComponent"));
	CombatReactionComponent = CreateDefaultSubobject<UCombatReactionComponent>(TEXT("CombatReactionComponent"));
	DefenseComponent = CreateDefaultSubobject<UDefenseComponent>(TEXT("DefenseComponent"));
}

void ABase::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->SpawnDefaultWeapon();
	}

}

void ABase::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->SetCurrentWeapon(NewWeapon);
	}
}

AWeaponBase* ABase::GetCurrentWeapon() const
{
	return WeaponHolderComponent ? WeaponHolderComponent->GetCurrentWeapon() : nullptr;
}

void ABase::NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier, float CounterAttackValidWindow)
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->NotifyWeaponAttackStarted(
			AttackDirection,
			AttackType,
			AttackStartTime,
			BaseDamage,
			DamageModifier,
			CounterAttackValidWindow
		);
	}
}

void ABase::NotifyWeaponAttackFinished(bool bInterrupted)
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->NotifyWeaponAttackFinished(bInterrupted);
	}
}

float ABase::GetMaxHealth() const
{
	return BaseHealthComponent ? BaseHealthComponent->GetMaxHealth() : 0.0f;
}

float ABase::GetCurrentHealth() const
{
	return BaseHealthComponent ? BaseHealthComponent->GetCurrentHealth() : 0.0f;
}

float ABase::GetHealthPercent() const
{
	return BaseHealthComponent ? BaseHealthComponent->GetHealthPercent() : 0.0f;
}

bool ABase::IsDead() const
{
	return !BaseHealthComponent || BaseHealthComponent->IsDead();
}

void ABase::Treat(float Treatmentamount)
{
	if (BaseHealthComponent)
	{
		BaseHealthComponent->Treat(Treatmentamount);
	}
}

float ABase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!BaseHealthComponent)
	{
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	if (BaseHealthComponent->IsDead() || DamageAmount <= 0.0f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[攻击交互][扣血忽略] 受击者=%s 当前血量=%.2f 输入伤害=%.2f 伤害来源=%s 控制器=%s"),
			*GetName(),
			GetCurrentHealth(),
			DamageAmount,
			*BaseSafeActorName(DamageCauser),
			EventInstigator ? *EventInstigator->GetName() : TEXT("无"));
		return 0.0f;
	}

	float OldHealth = 0.0f;
	float ActualDamage = 0.0f;
	BaseHealthComponent->ApplyDamage(DamageAmount, OldHealth, ActualDamage);

	Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	const bool bShouldSuppressHitReaction = BaseHealthComponent->ConsumeNextNonLethalHitReactionSuppressed();

	if (!BaseHealthComponent->IsDead())
	{
		if (!bShouldSuppressHitReaction)
		{
			PlayHitReaction();
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[攻击交互][扣血但跳过Hit反应] 受击者=%s 实际扣血=%.2f 原因=错误方向格挡仍播放Guard"),
				*GetName(),
				ActualDamage);
		}
	}
	else
	{
		PlayCombatReaction(
			ECombatReactionType::Death,
			EWeaponContactResult::Hit,
			EAttackDirection::None,
			false,
			false
		);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][扣血结算] 受击者=%s 伤害来源=%s 控制器=%s 输入伤害=%.2f 实际扣血=%.2f 血量=%.2f -> %.2f / %.2f"),
		*GetName(),
		*BaseSafeActorName(DamageCauser),
		EventInstigator ? *EventInstigator->GetName() : TEXT("无"),
		DamageAmount,
		ActualDamage,
		OldHealth,
		GetCurrentHealth(),
		GetMaxHealth());

	if (IsDead())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[攻击交互][角色死亡] 死亡角色=%s 最后一击来源=%s 最后一击伤害=%.2f"),
			*GetName(),
			*BaseSafeActorName(DamageCauser),
			ActualDamage);
	}

	return ActualDamage;
}

void ABase::EnableWeaponTrace()
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->EnableWeaponTrace();
	}
}

void ABase::DisableWeaponTrace()
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->DisableWeaponTrace();
	}
}

void ABase::InterruptCurrentAttackByBodyHit(AActor* DamageCauser)
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->ForceStopWeaponInteraction(TEXT("身体被武器命中，本次攻击被打断"));
	}

	if (UCombatComponent* CombatComponent = FindComponentByClass<UCombatComponent>())
	{
		CombatComponent->InterruptCurrentAttack();
	}
	else
	{
		CancelActiveAttack(TEXT("身体被武器命中，本次攻击被打断"));
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][角色攻击被打断] 角色=%s 打断来源=%s"),
		*GetName(),
		DamageCauser ? *DamageCauser->GetName() : TEXT("无"));
}

bool ABase::PlayCombatReaction(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse
)
{
	return CombatReactionComponent
		? CombatReactionComponent->PlayCombatReaction(ReactionType, ContactResult, Direction, bSelfIsSlower, bValidTimedResponse)
		: false;
}

bool ABase::PlayCombatReactionAndGetLength(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse,
	float& OutPlayedLength
)
{
	return CombatReactionComponent
		? CombatReactionComponent->PlayCombatReactionAndGetLength(ReactionType, ContactResult, Direction, bSelfIsSlower, bValidTimedResponse, OutPlayedLength)
		: false;
}

void ABase::PlayWeaponContactReaction(
	const FWeaponContactResolveOutput& ResolveOutput,
	EWeaponContactSide SelfSide
)
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->PlayWeaponContactReaction(ResolveOutput, SelfSide);
	}
}

void ABase::PlayHitReaction()
{
	if (CombatReactionComponent)
	{
		CombatReactionComponent->PlayHitReaction();
	}
}

void ABase::StartDeflect()
{
	if (DefenseComponent)
	{
		DefenseComponent->StartDeflect();
	}
}

void ABase::EndDeflect()
{
	if (DefenseComponent)
	{
		DefenseComponent->EndDeflect();
	}
}

bool ABase::IsDeflecting() const
{
	return DefenseComponent && DefenseComponent->IsDeflecting();
}

ABase* ABase::FindPreAttackGuardOpponent() const
{
	return DefenseComponent ? DefenseComponent->FindPreAttackGuardOpponent() : nullptr;
}

bool ABase::TryConvertAttackToGuard(
	EAttackDirection GuardDirection,
	float GuardRequestTime,
	float& OutGuardDuration)
{
	return DefenseComponent
		? DefenseComponent->TryConvertAttackToGuard(GuardDirection, GuardRequestTime, OutGuardDuration)
		: false;
}

void ABase::GrantNextAttackSpeedBonus(float PlayRateMultiplier)
{
	if (DefenseComponent)
	{
		DefenseComponent->GrantNextAttackSpeedBonus(PlayRateMultiplier);
	}
}

float ABase::ConsumeNextAttackPlayRateModifier()
{
    if (!DefenseComponent)
    {
        return 1.0f;
    }

    const float Result = DefenseComponent->ConsumeNextAttackPlayRateModifier();
    return Result;
}

void ABase::CancelCurrentAttackByGuard(AActor* GuardActor, const FString& Reason)
{
	if (WeaponHolderComponent)
	{
		WeaponHolderComponent->ForceStopWeaponInteraction(Reason);
	}

	if (UCombatComponent* CombatComponent = FindComponentByClass<UCombatComponent>())
	{
		CombatComponent->InterruptCurrentAttack();
	}

	CancelActiveAttack(Reason);
	OnAttackCancelledByGuard(GuardActor, Reason);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][攻击被格挡取消] 被取消者=%s 格挡者=%s 原因=%s"),
		*GetName(),
		*BaseSafeActorName(GuardActor),
		*Reason);
}

void ABase::ApplyDamageWithoutNonLethalHitReaction(
	float DamageAmount,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (DamageAmount <= 0.0f)
	{
		return;
	}

	if (BaseHealthComponent)
	{
		BaseHealthComponent->SuppressNextNonLethalHitReaction();
	}

	UGameplayStatics::ApplyDamage(
		this,
		DamageAmount,
		EventInstigator,
		DamageCauser,
		UDamageType::StaticClass()
	);

	// 如果 TakeDamage 因非法伤害等原因没有消耗该标记，这里兜底清掉。
	if (BaseHealthComponent)
	{
		BaseHealthComponent->ConsumeNextNonLethalHitReactionSuppressed();
	}
}

void ABase::CancelActiveAttack(const FString& Reason)
{
	(void)Reason;
}

void ABase::OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason)
{
	(void)GuardActor;
	(void)Reason;
}