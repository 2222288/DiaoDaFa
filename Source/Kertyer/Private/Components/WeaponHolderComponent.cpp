#include "Components/WeaponHolderComponent.h"
#include "Character/Base.h"
#include "Weapon/WeaponBase.h"
#include "Engine/World.h"

UWeaponHolderComponent::UWeaponHolderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	WeaponAttachSocketName = TEXT("hand_r_weapons");
}


void UWeaponHolderComponent::SpawnDefaultWeapon()
{
	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase)
	{
		return;
	}

	if (!CurrentWeapon && DefaultWeaponClass && OwnerBase->GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerBase;
		SpawnParams.Instigator = OwnerBase;

		AWeaponBase* SpawnedWeapon = OwnerBase->GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, OwnerBase->GetActorTransform(), SpawnParams);
		SetCurrentWeapon(SpawnedWeapon);
	}
	else if (CurrentWeapon)
	{
		SetCurrentWeapon(CurrentWeapon);
	}
}

void UWeaponHolderComponent::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
	ABase* OwnerBase = Cast<ABase>(GetOwner());
	CurrentWeapon = NewWeapon;

	if (!OwnerBase || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->SetCurrentHolder(OwnerBase);
	CurrentWeapon->AttachToComponent(
		OwnerBase->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponAttachSocketName
	);
}

void UWeaponHolderComponent::NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier, float CounterAttackValidWindow)
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[攻击交互][武器数据下发失败] 角色=%s 原因=CurrentWeapon为空"),
			*GetNameSafe(GetOwner()));
		return;
	}

	FWeaponAttackData AttackData;
	AttackData.AttackDirection = AttackDirection;
	AttackData.AttackStartTime = AttackStartTime;
	AttackData.AttackType = AttackType;
	AttackData.BaseDamage = BaseDamage;
	AttackData.DamageModifier = DamageModifier;
	AttackData.CounterAttackValidWindow = CounterAttackValidWindow > 0.0f ? CounterAttackValidWindow : 0.5f;

	CurrentWeapon->ReceiveAttackData(AttackData);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器数据下发] 角色=%s 武器=%s 攻击ID=%s 开始时间=%.3f 基础伤害=%.2f 倍率=%.3f 响应窗口=%.3f"),
		*GetNameSafe(GetOwner()),
		*CurrentWeapon->GetName(),
		*AttackType.ToString(),
		AttackStartTime,
		BaseDamage,
		DamageModifier,
		AttackData.CounterAttackValidWindow);
}

void UWeaponHolderComponent::NotifyWeaponAttackFinished(bool bInterrupted)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->CompleteAttackCycle(bInterrupted);
	}
}

void UWeaponHolderComponent::EnableWeaponTrace()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->EnableWeaponTrace();
	}
}

void UWeaponHolderComponent::DisableWeaponTrace()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->DisableWeaponTrace();
	}
}

void UWeaponHolderComponent::ForceStopWeaponInteraction(const FString& Reason)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->ForceStopWeaponInteraction(Reason);
	}
}