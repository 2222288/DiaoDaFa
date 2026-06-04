#include "Components/AttackComponent.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "DataAsset/AttackDH.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Character/Base.h"

namespace
{
	const TCHAR* AttackDirectionToChinese(EAttackDirection Direction)
	{
		switch (Direction)
		{
		case EAttackDirection::None: return TEXT("无");
		case EAttackDirection::Up: return TEXT("上");
		case EAttackDirection::UpRight: return TEXT("右上");
		case EAttackDirection::Right: return TEXT("右");
		case EAttackDirection::DownRight: return TEXT("右下");
		case EAttackDirection::Down: return TEXT("下");
		case EAttackDirection::DownLeft: return TEXT("左下");
		case EAttackDirection::Left: return TEXT("左");
		case EAttackDirection::UpLeft: return TEXT("左上");
		default: return TEXT("未知方向");
		}
	}

	const TCHAR* AttackStateToChinese(EAttackState State)
	{
		switch (State)
		{
		case EAttackState::Idle: return TEXT("空闲");
		case EAttackState::Sampling: return TEXT("采样中");
		case EAttackState::AttackingLocked: return TEXT("攻击锁定");
		case EAttackState::ComboWindowOpen: return TEXT("连击窗口打开");
		case EAttackState::SamplingLocked: return TEXT("采样但攻击锁定");
		case EAttackState::SamplingComboWindow: return TEXT("采样且连击窗口打开");
		default: return TEXT("未知状态");
		}
	}

	FString SafeActorName(const AActor* Actor)
	{
		return IsValid(Actor) ? Actor->GetName() : TEXT("无");
	}

	FString SafeNameText(FName Name)
	{
		return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
	}
}

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	AttackState = EAttackState::Idle;
	CurrentAttackStartTime = -1.f;
	CurrentAttackEndTime = -1.f;
	CurrentWindowTime = 0.0f;
	CurrentDirection = EAttackDirection::None;

	ClearPendingAttack();
	ClearSamplingBuffer();
}

void UAttackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();

	RefreshAttackState(CurrentTime);

	if (bHasPendingAttack && !IsLockedState())
	{
		PerformAttack(PendingDirection, PendingTrackScore);
	}

	const bool bAttackActive = HasActiveAttack(CurrentTime);

	if (!bAttackActive && bWeaponTraceWindowOpen)
	{
		DisableWeaponTrace();
	}

	if (!bAttackActive && !bHasPendingAttack)
	{
		CurrentDirection = EAttackDirection::None;
	}
}

void UAttackComponent::BeginAttackSampling(float CurrentTime)
{
	RefreshAttackState(CurrentTime);

	if (bIsAttackKeyDown)
	{
		return;
	}

	if (IsLockedState())
	{
		return;
	}

	if (!FAttackAnimationPlayer::ResolveAnimInstance(GetOwner()))
	{
		return;
	}

	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;
	bIsAttackKeyDown = true;

	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

void UAttackComponent::EndAttackSampling()
{
	bIsAttackKeyDown = false;
	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;

	ClearSamplingBuffer();

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

	RefreshAttackState(CurrentTime);
}

void UAttackComponent::CacheMouseInput(const FVector2D& Input, float CurrentTime)
{
	if (!IsSamplingState())
	{
		return;
	}

	FAttackValidResult Result;
	if (!AttackValid.PushInput(Input, CurrentTime, MinSampleDistance, Result))
	{
		return;
	}

	if (!Result.bCanTriggerAttack || Result.Direction == EAttackDirection::None)
	{
		return;
	}

	if (!CanAcceptAttackInput(Result.Direction, CurrentTime))
	{
		ClearSamplingBuffer();
		return;
	}

	MarkAttackInputAccepted(Result.Direction, CurrentTime);
	PerformAttack(Result.Direction, Result.TrackScore);
}

void UAttackComponent::PerformAttack(EAttackDirection Direction, float TrackScore)
{
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

	RefreshAttackState(CurrentTime);

	if (IsLockedState() && Direction == CurrentDirection)
	{
		ClearSamplingBuffer();
		return;
	}

	if (IsLockedState())
	{
		bHasPendingAttack = true;
		PendingDirection = Direction;
		PendingTrackScore = TrackScore;

		ClearSamplingBuffer();
		return;
	}

	const FAttack* AttackRow = FindAttackRowByDirection(Direction);
	if (!AttackRow || !AttackRow->AttackMontage)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[攻击交互][出招失败] 原因=未找到AttackRow或Montage为空 角色=%s 方向=%d"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			static_cast<int32>(Direction)
		);

		return;
	}

	const FAttackAnimationPlayResult AnimationResult =
		FAttackAnimationPlayer::PlayAttackMontage(GetOwner(), *AttackRow, 1.0f);

	if (!AnimationResult.bSucceeded)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[攻击交互][出招失败] 原因=%s 角色=%s Montage=%s Section=%s 方向=%d"),
			*AnimationResult.ErrorMessage,
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			*GetNameSafe(AttackRow->AttackMontage),
			*AttackRow->MontageSection.ToString(),
			static_cast<int32>(Direction)
		);

		return;
	}

	const float PlayedLength = AnimationResult.PlayedLength;

	CurrentBaseDamage = AttackRow->Damage;
	CurrentDamageModifier = NextAttackDamageModifier;
	NextAttackDamageModifier = TrackScore;
	CurrentWindowTime = AttackRow->WindowTime;
	CurrentAttackStartTime = CurrentTime;
	CurrentAttackEndTime = CurrentAttackStartTime + PlayedLength;
	CurrentDirection = Direction;
	CurrentAttackType = AttackRow->AttackID;
	bWeaponTraceWindowOpen = false;
	AttackTriggerCounter++;

	if (ABase* OwnerCharacter = Cast<ABase>(GetOwner()))
	{
		OwnerCharacter->NotifyWeaponAttackStarted(
			Direction,
			CurrentAttackType,
			CurrentAttackStartTime,
			CurrentBaseDamage,
			CurrentDamageModifier,
			CounterAttackValidWindow
		);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
		static_cast<int32>(Direction),
		*CurrentAttackType.ToString(),
		*GetNameSafe(AttackRow->AttackMontage),
		*AttackRow->MontageSection.ToString(),
		CurrentBaseDamage,
		CurrentDamageModifier,
		GetCurrentAttackDamage(),
		CurrentAttackStartTime,
		PlayedLength,
		CounterAttackValidWindow,
		AttackTriggerCounter
	);

	ClearPendingAttack();
	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

const FAttack* UAttackComponent::FindAttackRowByDirection(EAttackDirection InDirection) const
{
	if (!AttackDataTable || InDirection == EAttackDirection::None)
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("FindAttackRowByDirection"));

	const TArray<FName> RowNames = AttackDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FAttack* Row = AttackDataTable->FindRow<FAttack>(RowName, ContextString);
		if (!Row)
		{
			continue;
		}

		if (Row->AttackDirection == InDirection)
		{
			return Row;
		}
	}

	return nullptr;
}

void UAttackComponent::StartBlock()
{
	bIsBlocking = true;
}

void UAttackComponent::StopBlock()
{
	bIsBlocking = false;
}

void UAttackComponent::RefreshAttackState(float CurrentTime)
{
	const bool bActiveAttack = HasActiveAttack(CurrentTime);

	if (!bActiveAttack)
	{
		CurrentAttackStartTime = -1.f;
		CurrentAttackEndTime = -1.f;
		CurrentWindowTime = 0.0f;

		AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
		return;
	}

	const float Window = FMath::Max(0.1f, CurrentWindowTime);
	const float WindowStartTime = CurrentAttackStartTime + Window;
	const bool bWindowOpen = CurrentTime >= WindowStartTime;

	if (bIsAttackKeyDown)
	{
		AttackState = bWindowOpen
			? EAttackState::SamplingComboWindow
			: EAttackState::SamplingLocked;
	}
	else
	{
		AttackState = bWindowOpen
			? EAttackState::ComboWindowOpen
			: EAttackState::AttackingLocked;
	}
}

bool UAttackComponent::HasActiveAttack(float CurrentTime) const
{
	return CurrentAttackEndTime > 0.0f && CurrentTime < CurrentAttackEndTime;
}

bool UAttackComponent::IsSamplingState() const
{
	return AttackState == EAttackState::Sampling
		|| AttackState == EAttackState::SamplingLocked
		|| AttackState == EAttackState::SamplingComboWindow;
}

bool UAttackComponent::IsLockedState() const
{
	return AttackState == EAttackState::AttackingLocked
		|| AttackState == EAttackState::SamplingLocked;
}

void UAttackComponent::ClearSamplingBuffer()
{
	AttackValid.Reset();
}

void UAttackComponent::ClearPendingAttack()
{
	bHasPendingAttack = false;
	PendingDirection = EAttackDirection::None;
	PendingTrackScore = 0.0f;
}

bool UAttackComponent::CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const
{
	if (Direction == EAttackDirection::None)
	{
		return false;
	}

	if (LastAcceptedInputDirection == EAttackDirection::None)
	{
		return true;
	}

	const float Elapsed = CurrentTime - LastAcceptedInputTime;

	if (Direction == LastAcceptedInputDirection)
	{
		return false;
	}

	if (Elapsed < DirectionSwitchCooldown)
	{
		return false;
	}

	if (Elapsed < AttackRequestCooldown)
	{
		return false;
	}

	return true;
}

void UAttackComponent::MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime)
{
	LastAcceptedInputDirection = Direction;
	LastAcceptedInputTime = CurrentTime;
}

bool UAttackComponent::IsAttackActive() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return HasActiveAttack(World->GetTimeSeconds());
}

float UAttackComponent::GetCurrentAttackDamage() const
{
	return CurrentBaseDamage * CurrentDamageModifier;
}

void UAttackComponent::EnableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = true;
	OwnerCharacter->EnableWeaponTrace();
}

void UAttackComponent::DisableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = false;
	OwnerCharacter->DisableWeaponTrace();
}

void UAttackComponent::InterruptCurrentAttack()
{
	FAttackAnimationPlayer::StopAttackMontage(GetOwner(),nullptr,0.1f);

	CurrentAttackStartTime = -1.0f;
	CurrentAttackEndTime = -1.0f;
	CurrentWindowTime = 0.0f;
	CurrentDirection = EAttackDirection::None;
	CurrentAttackType = NAME_None;
	CurrentBaseDamage = 0.0f;
	bWeaponTraceWindowOpen = false;

	ClearPendingAttack();
	ClearSamplingBuffer();

	AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][攻击组件被打断] 角色=%s 新状态=%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
		static_cast<int32>(AttackState)
	);
}