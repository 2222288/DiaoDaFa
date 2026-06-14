#include "Components/CombatComponent.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "DataAsset/AttackMoveDataAsset.h"
#include "Engine/World.h"
#include "Combat/CombatStatusSwitch.h"
#include "Character/Base.h"

////日志调用
//namespace
//{
//	const TCHAR* AttackDirectionToChinese(EAttackDirection Direction)
//	{
//		switch (Direction)
//		{
//		case EAttackDirection::None: return TEXT("无");
//		case EAttackDirection::Up: return TEXT("上");
//		case EAttackDirection::UpRight: return TEXT("右上");
//		case EAttackDirection::Right: return TEXT("右");
//		case EAttackDirection::DownRight: return TEXT("右下");
//		case EAttackDirection::Down: return TEXT("下");
//		case EAttackDirection::DownLeft: return TEXT("左下");
//		case EAttackDirection::Left: return TEXT("左");
//		case EAttackDirection::UpLeft: return TEXT("左上");
//		default: return TEXT("未知方向");
//		}
//	}
//
//	const TCHAR* AttackStateToChinese(EAttackState State)
//	{
//		switch (State)
//		{
//		case EAttackState::Idle: return TEXT("空闲");
//		case EAttackState::Sampling: return TEXT("采样中");
//		case EAttackState::AttackingLocked: return TEXT("攻击锁定");
//		case EAttackState::ComboWindowOpen: return TEXT("连击窗口打开");
//		case EAttackState::SamplingLocked: return TEXT("采样但攻击锁定");
//		case EAttackState::SamplingComboWindow: return TEXT("采样且连击窗口打开");
//		default: return TEXT("未知状态");
//		}
//	}
//
//	FString SafeActorName(const AActor* Actor)
//	{
//		return IsValid(Actor) ? Actor->GetName() : TEXT("无");
//	}
//
//	FString SafeNameText(FName Name)
//	{
//		return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
//	}
//}

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CombatStatusSwitch.AttackState = EAttackState::Idle;
	CombatStatusSwitch.CurrentAttackStartTime = -1.f;
	CombatStatusSwitch.CurrentAttackEndTime = -1.f;
	CombatStatusSwitch.CurrentWindowTime = 0.0f;
	CombatStatusSwitch.CurrentDirection = EAttackDirection::None;

	CombatSampling.ClearPendingAttack();
	CombatSampling.ClearSamplingBuffer();
}

void UCombatComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!World && !Owner)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();

	CombatStatusSwitch.GetActor(CurrentTime, Owner, World);
	
	CombatStatusSwitch.RefreshAttackState(CurrentTime);

	if (CombatStatusSwitch.bHasPendingAttack && !CombatStatusSwitch.IsLockedState())
	{
		CombatDamage.PerformAttack(CombatStatusSwitch.PendingDirection, CombatSampling.PendingTrackScore);
	}

	const bool bAttackActive = CombatStatusSwitch.HasActiveAttack(CurrentTime);

	if (!bAttackActive && CombatDamage.bWeaponTraceWindowOpen)
	{
		CombatDamage.DisableWeaponTrace();
	}

	if (!bAttackActive && !CombatStatusSwitch.bHasPendingAttack)
	{
		CombatStatusSwitch.CurrentDirection = EAttackDirection::None;
	}
}

//void UCombatComponent::BeginAttackSampling(float CurrentTime)
//{
//	CombatStatusSwitch.RefreshAttackState(CurrentTime);
//
//	if (bIsAttackKeyDown)
//	{
//		return;
//	}
//
//	if (CombatStatusSwitch.IsLockedState())
//	{
//		return;
//	}
//
//	if (!FAttackAnimationPlayer::ResolveAnimInstance(GetOwner()))
//	{
//		return;
//	}
//
//	CombatStatusSwitch.LastAcceptedInputDirection = EAttackDirection::None;
//	CombatStatusSwitch.LastAcceptedInputTime = -10000.0f;
//	bIsAttackKeyDown = true;
//
//	ClearSamplingBuffer();
//	CombatStatusSwitch.RefreshAttackState(CurrentTime);
//}
//
//void UCombatComponent::EndAttackSampling()
//{
//	bIsAttackKeyDown = false;
//	CombatStatusSwitch.LastAcceptedInputDirection = EAttackDirection::None;
//	CombatStatusSwitch.LastAcceptedInputTime = -10000.0f;
//
//	ClearSamplingBuffer();
//
//	UWorld* World = GetWorld();
//	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
//
//	CombatStatusSwitch.RefreshAttackState(CurrentTime);
//}
//
//void UCombatComponent::CacheMouseInput(const FVector2D& Input, float CurrentTime)
//{
//	if (!CombatStatusSwitch.IsSamplingState())
//	{
//		return;
//	}
//
//	FAttackValidResult Result;
//	if (!CombatStatusSwitch.AttackValid.PushInput(Input, CurrentTime, MinSampleDistance, Result))
//	{
//		return;
//	}
//
//	if (!Result.bCanTriggerAttack || Result.Direction == EAttackDirection::None)
//	{
//		return;
//	}
//
//	if (!CombatStatusSwitch.CanAcceptAttackInput(Result.Direction, CurrentTime))
//	{
//		ClearSamplingBuffer();
//		return;
//	}
//
//	CombatStatusSwitch.MarkAttackInputAccepted(Result.Direction, CurrentTime);
//	PerformAttack(Result.Direction, Result.TrackScore);
//}

//void UCombatComponent::PerformAttack(EAttackDirection Direction, float TrackScore)
//{
//	ABase* OwnerBase = Cast<ABase>(GetOwner());
//	if (OwnerBase && OwnerBase->IsDeflecting())
//	{
//		UE_LOG(
//			LogTemp,
//			Warning,
//			TEXT("[攻击交互][攻击失败] 当前正在弹刀，不能攻击 角色=%s"),
//			GetOwner() ? *GetOwner()->GetName() : TEXT("无")
//		);
//
//		return;
//	}
//
//	if (Direction == EAttackDirection::None)
//	{
//		return;
//	}
//
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return;
//	}
//
//	const float CurrentTime = World->GetTimeSeconds();
//
//	CombatStatusSwitch.RefreshAttackState(CurrentTime);
//
//	if (CombatStatusSwitch.IsLockedState() && Direction == CombatStatusSwitch.CurrentDirection)
//	{
//		CombatSampling.ClearSamplingBuffer();
//		return;
//	}
//
//	if (CombatStatusSwitch.IsLockedState())
//	{
//		CombatStatusSwitch.bHasPendingAttack = true;
//		CombatStatusSwitch.PendingDirection = Direction;
//		CombatSampling.PendingTrackScore = TrackScore;
//
//		CombatSampling.ClearSamplingBuffer();
//		return;
//	}
//
//	const FAttackMoveData* AttackData = CombatSampling.FindAttackMoveByDirection(Direction);
//	if (!AttackData || !AttackData->AttackMontage)
//	{
//		UE_LOG(
//			LogTemp,
//			Error,
//			TEXT("[攻击交互][出招失败] 原因=未找到攻击动作数据或Montage为空 角色=%s 方向=%d"),
//			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//			static_cast<int32>(Direction)
//		);
//
//		return;
//	}
//
//	float GuardDuration = 0.0f;
//
//	if (OwnerBase && OwnerBase->TryConvertAttackToGuard(Direction, CurrentTime, GuardDuration))
//	{
//		CombatStatusSwitch.StartConvertedGuard(Direction, CurrentTime, GuardDuration);
//		CombatSampling.ClearPendingAttack();
//		CombatSampling.ClearSamplingBuffer();
//		CombatStatusSwitch.RefreshAttackState(CurrentTime);
//		return;
//	}
//
//	const float AttackPlayRate = OwnerBase
//		? OwnerBase->ConsumeNextAttackPlayRateModifier()
//		: 1.0f;
//
//	const FAttackAnimationPlayResult AnimationResult =
//		FAttackAnimationPlayer::PlayAttackMontage(GetOwner(), *AttackData, AttackPlayRate);
//
//	if (!AnimationResult.bSucceeded)
//	{
//		if (OwnerBase && AttackPlayRate > 1.0f)
//		{
//			OwnerBase->GrantNextAttackSpeedBonus(AttackPlayRate);
//		}
//
//		UE_LOG(
//			LogTemp,
//			Error,
//			TEXT("[攻击交互][出招失败] 原因=%s 角色=%s Montage=%s Section=%s 方向=%d"),
//			*AnimationResult.ErrorMessage,
//			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//			*GetNameSafe(AttackData->AttackMontage),
//			*AttackData->MontageSection.ToString(),
//			static_cast<int32>(Direction)
//		);
//
//		return;
//	}
//
//	const float PlayedLength = AnimationResult.PlayedLength;
//
//	const auto ApplySuccessfulAttackState = [&]()
//		{
//			CombatStatusSwitch.bIsBlocking = false;
//
//			CurrentBaseDamage = AttackData->Damage;
//			CombatDamage.CurrentDamageModifier = CombatDamage.NextAttackDamageModifier;
//			CombatDamage.NextAttackDamageModifier = TrackScore;
//			CombatStatusSwitch.CurrentWindowTime = AttackData->WindowTime;
//			CombatStatusSwitch.CurrentAttackStartTime = CurrentTime;
//			CombatStatusSwitch.CurrentAttackEndTime = CombatStatusSwitch.CurrentAttackStartTime + PlayedLength;
//			CombatStatusSwitch.CurrentDirection = Direction;
//			CurrentAttackType = AttackData->AttackID;
//			bWeaponTraceWindowOpen = false;
//			CombatStatusSwitch.AttackTriggerCounter++;
//		};
//
//	const auto NotifyWeaponAttackStartedIfOwnerIsBase = [&](float ActualCounterAttackValidWindow) -> bool
//		{
//			if (ABase* OwnerCharacter = Cast<ABase>(GetOwner()))
//			{
//				OwnerCharacter->NotifyWeaponAttackStarted(
//					Direction,
//					CurrentAttackType,
//					CombatStatusSwitch.CurrentAttackStartTime,
//					CurrentBaseDamage,
//					CombatDamage.CurrentDamageModifier,
//					ActualCounterAttackValidWindow
//				);
//
//				return true;
//			}
//
//			return false;
//		};
//
//	const auto LogSuccessfulAttackWithPlayRate = [&](float ActualCounterAttackValidWindow)
//		{
//			UE_LOG(
//				LogTemp,
//				Warning,
//				TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击播放倍率=%.3f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
//				GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//				static_cast<int32>(Direction),
//				*CurrentAttackType.ToString(),
//				*GetNameSafe(AttackData->AttackMontage),
//				*AttackData->MontageSection.ToString(),
//				CurrentBaseDamage,
//				CombatDamage.CurrentDamageModifier,
//				CombatDamage.GetCurrentAttackDamage(),
//				AttackPlayRate,
//				CombatStatusSwitch.CurrentAttackStartTime,
//				PlayedLength,
//				ActualCounterAttackValidWindow,
//				CombatStatusSwitch.AttackTriggerCounter
//			);
//		};
//
//	const auto LogSuccessfulAttackWithoutPlayRate = [&](float ActualCounterAttackValidWindow)
//		{
//			UE_LOG(
//				LogTemp,
//				Warning,
//				TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
//				GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//				static_cast<int32>(Direction),
//				*CurrentAttackType.ToString(),
//				*GetNameSafe(AttackData->AttackMontage),
//				*AttackData->MontageSection.ToString(),
//				CurrentBaseDamage,
//				CombatDamage.CurrentDamageModifier,
//				CombatDamage.GetCurrentAttackDamage(),
//				CombatStatusSwitch.CurrentAttackStartTime,
//				PlayedLength,
//				ActualCounterAttackValidWindow,
//				CombatStatusSwitch.AttackTriggerCounter
//			);
//		};
//
//	ApplySuccessfulAttackState();
//
//	const float ActualCounterAttackValidWindow =
//		AttackData->CounterAttackValidWindow > 0.0f
//		? AttackData->CounterAttackValidWindow
//		: CombatStatusSwitch.CounterAttackValidWindow;
//
//	if (NotifyWeaponAttackStartedIfOwnerIsBase(ActualCounterAttackValidWindow))
//	{
//		LogSuccessfulAttackWithPlayRate(ActualCounterAttackValidWindow);
//		return;
//	}
//
//	ApplySuccessfulAttackState();
//
//	NotifyWeaponAttackStartedIfOwnerIsBase(ActualCounterAttackValidWindow);
//	LogSuccessfulAttackWithoutPlayRate(ActualCounterAttackValidWindow);
//
//	CombatSampling.ClearPendingAttack();
//	CombatSampling.ClearSamplingBuffer();
//	CombatStatusSwitch.RefreshAttackState(CurrentTime);
//}
//void UCombatComponent::StartConvertedGuard(
//	EAttackDirection Direction,
//	float CurrentTime,
//	float GuardDuration
//)
//{
//	const float SafeGuardDuration = FMath::Max(0.1f, GuardDuration);
//
//	CombatStatusSwitch.bIsBlocking = true;
//	bIsDeflecting = false;
//
//	CurrentBaseDamage = 0.0f;
//	CurrentDamageModifier = 1.0f;
//	CombatStatusSwitch.CurrentWindowTime = SafeGuardDuration;
//	CombatStatusSwitch.CurrentAttackStartTime = CurrentTime;
//	CombatStatusSwitch.CurrentAttackEndTime = CurrentTime + SafeGuardDuration;
//	CombatStatusSwitch.CurrentDirection = Direction;
//	CurrentAttackType = TEXT("Guard");
//	bWeaponTraceWindowOpen = false;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][攻击转格挡] 角色=%s 方向=%s 开始=%.3f 时长=%.3f"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//		AttackDirectionToChinese(Direction),
//		CombatStatusSwitch.CurrentAttackStartTime,
//		SafeGuardDuration
//	);
//}

//const FAttackMoveData* UCombatComponent::FindAttackMoveByDirection(EAttackDirection InDirection) const
//{
//	if (!AttackMoveDataAsset || InDirection == EAttackDirection::None)
//	{
//		return nullptr;
//	}
//
//	return AttackMoveDataAsset->FindAttackByDirection(InDirection);
//}

//void UCombatComponent::StartBlock()
//{
//	if (CombatStatusSwitch.bIsBlocking)
//	{
//		return;
//	}
//
//	CombatStatusSwitch.bIsBlocking = true;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][格挡开始] 角色=%s 当前攻击状态=%d"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//		static_cast<int32>(CombatStatusSwitch.AttackState)
//	);
//}
//
//void UCombatComponent::StopBlock()
//{
//	if (!CombatStatusSwitch.bIsBlocking)
//	{
//		return;
//	}
//
//	CombatStatusSwitch.bIsBlocking = false;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][格挡结束] 角色=%s 当前攻击状态=%d"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//		static_cast<int32>(CombatStatusSwitch.AttackState)
//	);
//}

//void UCombatComponent::CombatStatusSwitch.RefreshAttackState(float CurrentTime)
//{
//	const bool bActiveAttack = HasActiveAttack(CurrentTime);
//
//	if (!bActiveAttack)
//	{
//		CombatStatusSwitch.CurrentAttackStartTime = -1.f;
//		CombatStatusSwitch.CurrentAttackEndTime = -1.f;
//		CombatStatusSwitch.CurrentWindowTime = 0.0f;
//
//		AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
//		return;
//	}
//
//	const float Window = FMath::Max(0.1f, CombatStatusSwitch.CurrentWindowTime);
//	const float WindowStartTime = CombatStatusSwitch.CurrentAttackStartTime + Window;
//	const bool bWindowOpen = CurrentTime >= WindowStartTime;
//
//	if (bIsAttackKeyDown)
//	{
//		AttackState = bWindowOpen
//			? EAttackState::SamplingComboWindow
//			: EAttackState::SamplingLocked;
//	}
//	else
//	{
//		AttackState = bWindowOpen
//			? EAttackState::ComboWindowOpen
//			: EAttackState::AttackingLocked;
//	}
//}
//
//bool UCombatComponent::HasActiveAttack(float CurrentTime) const
//{
//	return CombatStatusSwitch.CurrentAttackEndTime > 0.0f && CurrentTime < CombatStatusSwitch.CurrentAttackEndTime;
//}

//bool UCombatComponent::IsSamplingState() const
//{
//	return AttackState == EAttackState::Sampling
//		|| AttackState == EAttackState::SamplingLocked
//		|| AttackState == EAttackState::SamplingComboWindow;
//}
//
//bool UCombatComponent::IsLockedState() const
//{
//	return AttackState == EAttackState::AttackingLocked
//		|| AttackState == EAttackState::SamplingLocked;
//}

//void UCombatComponent::ClearSamplingBuffer()
//{
//	CombatStatusSwitch.AttackValid.Reset();
//}
//
//void UCombatComponent::ClearPendingAttack()
//{
//	CombatStatusSwitch.bHasPendingAttack = false;
//	CombatStatusSwitch.PendingDirection = EAttackDirection::None;
//	PendingTrackScore = 0.0f;
//}

//bool UCombatComponent::CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const
//{
//	if (Direction == EAttackDirection::None)
//	{
//		return false;
//	}
//
//	if (LastAcceptedInputDirection == EAttackDirection::None)
//	{
//		return true;
//	}
//
//	const float Elapsed = CurrentTime - LastAcceptedInputTime;
//
//	if (Direction == LastAcceptedInputDirection)
//	{
//		return false;
//	}
//
//	if (Elapsed < DirectionSwitchCooldown)
//	{
//		return false;
//	}
//
//	if (Elapsed < AttackRequestCooldown)
//	{
//		return false;
//	}
//
//	return true;
//}

//void UCombatComponent::MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime)
//{
//	CombatStatusSwitch.LastAcceptedInputDirection = Direction;
//	CombatStatusSwitch.LastAcceptedInputTime = CurrentTime;
//}

//bool UCombatComponent::IsAttackActive() const
//{
//	UWorld* World = GetWorld();
//	if (!World)
//	{
//		return false;
//	}
//
//	return CombatStatusSwitch.HasActiveAttack(World->GetTimeSeconds());
//}

//float UCombatComponent::GetCurrentAttackDamage() const
//{
//	return CurrentBaseDamage * CurrentDamageModifier;
//}
//
//void UCombatComponent::EnableWeaponTrace()
//{
//	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
//	if (!OwnerCharacter)
//	{
//		return;
//	}
//
//	bWeaponTraceWindowOpen = true;
//	OwnerCharacter->EnableWeaponTrace();
//}
//
//void UCombatComponent::DisableWeaponTrace()
//{
//	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
//	if (!OwnerCharacter)
//	{
//		return;
//	}
//
//	bWeaponTraceWindowOpen = false;
//	OwnerCharacter->DisableWeaponTrace();
//}

//void UCombatComponent::InterruptCurrentAttack()
//{
//	FAttackAnimationPlayer::StopAttackMontage(GetOwner(),nullptr,0.1f);
//
//	CombatStatusSwitch.bIsBlocking = false;
//	CombatStatusSwitch.bIsDeflecting = false;
//
//	CombatStatusSwitch.CurrentAttackStartTime = -1.0f;
//	CombatStatusSwitch.CurrentAttackEndTime = -1.0f;
//	CombatStatusSwitch.CurrentWindowTime = 0.0f;
//	CombatStatusSwitch.CurrentDirection = EAttackDirection::None;
//	CurrentAttackType = NAME_None;
//	CurrentBaseDamage = 0.0f;
//	bWeaponTraceWindowOpen = false;
//
//	CombatSampling.ClearPendingAttack();
//	CombatSampling.ClearSamplingBuffer();
//
//	CombatStatusSwitch.AttackState = CombatStatusSwitch.bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][攻击组件被打断] 角色=%s 新状态=%d"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
//		static_cast<int32>(CombatStatusSwitch.AttackState)
//	);
//}

//void UCombatComponent::StartDeflect()
//{
//	if (bIsDeflecting)
//	{
//		return;
//	}
//
//	bIsDeflecting = true;
//
//	// 弹刀时不应继续保持格挡有效帧
//	CombatStatusSwitch.bIsBlocking = false;
//
//	// 弹刀时当前攻击被打断，防止继续造成伤害
//	CombatStatusSwitch.CurrentAttackStartTime = -1.0f;
//	CurrentBaseDamage = 0.0f;
//	CurrentDamageModifier = 1.0f;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][弹刀开始] 角色=%s"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无")
//	);
//}
//
//void UCombatComponent::EndDeflect()
//{
//	if (!bIsDeflecting)
//	{
//		return;
//	}
//
//	bIsDeflecting = false;
//
//	UE_LOG(
//		LogTemp,
//		Warning,
//		TEXT("[攻击交互][弹刀结束] 角色=%s"),
//		GetOwner() ? *GetOwner()->GetName() : TEXT("无")
//	);
//}