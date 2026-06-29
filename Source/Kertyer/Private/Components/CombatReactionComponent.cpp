#include "Components/CombatReactionComponent.h"
#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Character/Base.h"
#include "DataAsset/CombatReactionAnimationData.h"
#include "Weapon/WeaponBase.h"

UCombatReactionComponent::UCombatReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


bool UCombatReactionComponent::PlayCombatReaction(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse
)
{
	float IgnoredPlayedLength = 0.0f;

	return PlayCombatReactionAndGetLength(
		ReactionType,
		ContactResult,
		Direction,
		bSelfIsSlower,
		bValidTimedResponse,
		IgnoredPlayedLength
	);
}

bool UCombatReactionComponent::PlayCombatReactionAndGetLength(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse,
	float& OutPlayedLength
)
{
	OutPlayedLength = 0.0f;

	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase || !CombatReactionAnimationData)
	{
		return false;
	}

	const FCombatReactionAnimation* ReactionRow =
		CombatReactionAnimationData->FindBestReaction(
			ReactionType,
			ContactResult,
			Direction,
			bSelfIsSlower,
			bValidTimedResponse
		);

	if (!ReactionRow)
	{
		return false;
	}

	const FAttackAnimationPlayResult Result =
		FAttackAnimationPlayer::PlayReactionMontage(OwnerBase, *ReactionRow);

	if (!Result.bSucceeded)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[战斗反应动画][播放失败] 角色=%s 类型=%d 原因=%s Montage=%s Section=%s"),
			*OwnerBase->GetName(),
			static_cast<int32>(ReactionType),
			*Result.ErrorMessage,
			*GetNameSafe(ReactionRow->Montage),
			*ReactionRow->MontageSection.ToString()
		);

		return false;
	}

	OutPlayedLength = Result.PlayedLength;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[战斗反应动画][播放成功] 角色=%s 类型=%d Montage=%s Section=%s 时长=%.3f"),
		*OwnerBase->GetName(),
		static_cast<int32>(ReactionType),
		*GetNameSafe(Result.PlayedMontage),
		*Result.PlayedSection.ToString(),
		Result.PlayedLength
	);

	return true;
}

void UCombatReactionComponent::PlayWeaponContactReaction(
	const FWeaponContactResolveOutput& ResolveOutput,
	EWeaponContactSide SelfSide
)
{
	if (SelfSide == EWeaponContactSide::None)
	{
		return;
	}

	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase)
	{
		return;
	}

	const bool bSelfIsSlower = ResolveOutput.SlowerSide == SelfSide;

	const bool bSelfIsDamaged =
		ResolveOutput.DamagedSide == SelfSide ||
		ResolveOutput.DamagedSide == EWeaponContactSide::Both;

	ECombatReactionType ReactionType = ECombatReactionType::None;

	switch (ResolveOutput.Result)
	{
	case EWeaponContactResult::Clash:
		// 非等速，方向能架住：
		// 较慢方播放格挡；
		// 较快方继续保持攻击动画，不强行切 Clash。
		if (bSelfIsSlower && ResolveOutput.bIsValidTimedResponse)
		{
			ReactionType = ECombatReactionType::Guard;
		}
		else
		{
			return;
		}
		break;

	case EWeaponContactResult::Deflect:
		ReactionType = ECombatReactionType::Deflect;
		OwnerBase->StartDeflect();
		break;

	case EWeaponContactResult::Interrupt:
		if (!bSelfIsDamaged)
		{
			return;
		}

		ReactionType = ECombatReactionType::Interrupt;
		break;

	case EWeaponContactResult::Hit:
		// 只有受伤方播放受击反应。
		// 优势方继续保持攻击动画。
		if (!bSelfIsDamaged)
		{
			return;
		}

		ReactionType = ECombatReactionType::Hit;
		break;

	case EWeaponContactResult::Ignore:
	default:
		return;
	}

	EAttackDirection ReactionDirection = EAttackDirection::None;

	if (AWeaponBase* CurrentWeapon = OwnerBase->GetCurrentWeapon())
	{
		ReactionDirection = CurrentWeapon->GetCurrentAttackDirection();
	}

	PlayCombatReaction(
		ReactionType,
		ResolveOutput.Result,
		ReactionDirection,
		bSelfIsSlower,
		ResolveOutput.bIsValidTimedResponse
	);
}

void UCombatReactionComponent::PlayHitReaction()
{
	PlayCombatReaction(
		ECombatReactionType::Hit,
		EWeaponContactResult::Hit,
		EAttackDirection::None,
		false,
		false
	);
}