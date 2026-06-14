#include "Combat/CombatDamage.h"

float FCombatDamage::GetCurrentAttackDamage() const
{
	return CurrentBaseDamage * CurrentDamageModifier;
}

void FCombatDamage::EnableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = true;
	OwnerCharacter->EnableWeaponTrace();
}

void FCombatDamage::DisableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = false;
	OwnerCharacter->DisableWeaponTrace();
}

void FCombatDamage::PerformAttack(EAttackDirection Direction, float TrackScore)
{
	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (OwnerBase && OwnerBase->IsDeflecting())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[攻击交互][攻击失败] 当前正在弹刀，不能攻击 角色=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无")
		);

		return;
	}

	if (Direction == EAttackDirection::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();

	CombatStatusSwitch.RefreshAttackState(CurrentTime);

	if (CombatStatusSwitch.IsLockedState() && Direction == CombatStatusSwitch.CurrentDirection)
	{
		CombatSampling.ClearSamplingBuffer();
		return;
	}

	if (CombatStatusSwitch.IsLockedState())
	{
		CombatStatusSwitch.bHasPendingAttack = true;
		CombatStatusSwitch.PendingDirection = Direction;
		CombatSampling.PendingTrackScore = TrackScore;

		CombatSampling.ClearSamplingBuffer();
		return;
	}

	const FAttackMoveData* AttackData = CombatSampling.FindAttackMoveByDirection(Direction);
	if (!AttackData || !AttackData->AttackMontage)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[攻击交互][出招失败] 原因=未找到攻击动作数据或Montage为空 角色=%s 方向=%d"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			static_cast<int32>(Direction)
		);

		return;
	}

	float GuardDuration = 0.0f;

	if (OwnerBase && OwnerBase->TryConvertAttackToGuard(Direction, CurrentTime, GuardDuration))
	{
		CombatStatusSwitch.StartConvertedGuard(Direction, CurrentTime, GuardDuration);
		CombatSampling.ClearPendingAttack();
		CombatSampling.ClearSamplingBuffer();
		CombatStatusSwitch.RefreshAttackState(CurrentTime);
		return;
	}

	const float AttackPlayRate = OwnerBase
		? OwnerBase->ConsumeNextAttackPlayRateModifier()
		: 1.0f;

	const FAttackAnimationPlayResult AnimationResult =
		FAttackAnimationPlayer::PlayAttackMontage(GetOwner(), *AttackData, AttackPlayRate);

	if (!AnimationResult.bSucceeded)
	{
		if (OwnerBase && AttackPlayRate > 1.0f)
		{
			OwnerBase->GrantNextAttackSpeedBonus(AttackPlayRate);
		}

		UE_LOG(
			LogTemp,
			Error,
			TEXT("[攻击交互][出招失败] 原因=%s 角色=%s Montage=%s Section=%s 方向=%d"),
			*AnimationResult.ErrorMessage,
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			*GetNameSafe(AttackData->AttackMontage),
			*AttackData->MontageSection.ToString(),
			static_cast<int32>(Direction)
		);

		return;
	}

	const float PlayedLength = AnimationResult.PlayedLength;

	const auto ApplySuccessfulAttackState = [&]()
		{
			CombatStatusSwitch.bIsBlocking = false;

			CurrentBaseDamage = AttackData->Damage;
			CombatDamage.CurrentDamageModifier = CombatDamage.NextAttackDamageModifier;
			CombatDamage.NextAttackDamageModifier = TrackScore;
			CombatStatusSwitch.CurrentWindowTime = AttackData->WindowTime;
			CombatStatusSwitch.CurrentAttackStartTime = CurrentTime;
			CombatStatusSwitch.CurrentAttackEndTime = CombatStatusSwitch.CurrentAttackStartTime + PlayedLength;
			CombatStatusSwitch.CurrentDirection = Direction;
			CurrentAttackType = AttackData->AttackID;
			bWeaponTraceWindowOpen = false;
			CombatStatusSwitch.AttackTriggerCounter++;
		};

	const auto NotifyWeaponAttackStartedIfOwnerIsBase = [&](float ActualCounterAttackValidWindow) -> bool
		{
			if (ABase* OwnerCharacter = Cast<ABase>(GetOwner()))
			{
				OwnerCharacter->NotifyWeaponAttackStarted(
					Direction,
					CurrentAttackType,
					CombatStatusSwitch.CurrentAttackStartTime,
					CurrentBaseDamage,
					CombatDamage.CurrentDamageModifier,
					ActualCounterAttackValidWindow
				);

				return true;
			}

			return false;
		};

	const auto LogSuccessfulAttackWithPlayRate = [&](float ActualCounterAttackValidWindow)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击播放倍率=%.3f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
				static_cast<int32>(Direction),
				*CurrentAttackType.ToString(),
				*GetNameSafe(AttackData->AttackMontage),
				*AttackData->MontageSection.ToString(),
				CurrentBaseDamage,
				CombatDamage.CurrentDamageModifier,
				CombatDamage.GetCurrentAttackDamage(),
				AttackPlayRate,
				CombatStatusSwitch.CurrentAttackStartTime,
				PlayedLength,
				ActualCounterAttackValidWindow,
				CombatStatusSwitch.AttackTriggerCounter
			);
		};

	const auto LogSuccessfulAttackWithoutPlayRate = [&](float ActualCounterAttackValidWindow)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
				static_cast<int32>(Direction),
				*CurrentAttackType.ToString(),
				*GetNameSafe(AttackData->AttackMontage),
				*AttackData->MontageSection.ToString(),
				CurrentBaseDamage,
				CombatDamage.CurrentDamageModifier,
				CombatDamage.GetCurrentAttackDamage(),
				CombatStatusSwitch.CurrentAttackStartTime,
				PlayedLength,
				ActualCounterAttackValidWindow,
				CombatStatusSwitch.AttackTriggerCounter
			);
		};

	ApplySuccessfulAttackState();

	const float ActualCounterAttackValidWindow =
		AttackData->CounterAttackValidWindow > 0.0f
		? AttackData->CounterAttackValidWindow
		: CombatStatusSwitch.CounterAttackValidWindow;

	if (NotifyWeaponAttackStartedIfOwnerIsBase(ActualCounterAttackValidWindow))
	{
		LogSuccessfulAttackWithPlayRate(ActualCounterAttackValidWindow);
		return;
	}

	ApplySuccessfulAttackState();

	NotifyWeaponAttackStartedIfOwnerIsBase(ActualCounterAttackValidWindow);
	LogSuccessfulAttackWithoutPlayRate(ActualCounterAttackValidWindow);

	CombatSampling.ClearPendingAttack();
	CombatSampling.ClearSamplingBuffer();
	CombatStatusSwitch.RefreshAttackState(CurrentTime);
}