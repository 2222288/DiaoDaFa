#include "Combat/CombatStatusSwitch.h"


//日志调用
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

void FCombatStatusSwitch::GetActor(float DeltaTime, AActor* Actor, UWorld* World)
{
	CSDeltaTime = DeltaTime;
	CSActor = Actor;
	CSWorld = World;
}

void FCombatStatusSwitch::RefreshAttackState(float CurrentTime)
{
	//是否存在未结束的攻击
	const bool bActiveAttack = HasActiveAttack(CurrentTime);

	if (!bActiveAttack)
	{
		CurrentAttackStartTime = -1.f;
		CurrentAttackEndTime = -1.f;
		CurrentWindowTime = 0.0f;

		AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
		return;
	}

	//连击窗口时间
	const float Window = FMath::Max(0.1f, CurrentWindowTime);
	//连击窗口开始时间
	const float WindowStartTime = CurrentAttackStartTime + Window;
	//连击窗口是否已经开始
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

bool FCombatStatusSwitch::HasActiveAttack(float CurrentTime) const
{
	return CurrentAttackEndTime > 0.0f && CurrentTime < CurrentAttackEndTime;
}


bool FCombatStatusSwitch::IsSamplingState() const
{
	return AttackState == EAttackState::Sampling
		|| AttackState == EAttackState::SamplingLocked
		|| AttackState == EAttackState::SamplingComboWindow;
}

bool FCombatStatusSwitch::IsLockedState() const
{
	return AttackState == EAttackState::AttackingLocked
		|| AttackState == EAttackState::SamplingLocked;
}

bool FCombatSampling::CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const
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

void FCombatStatusSwitch::MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime)
{
	LastAcceptedInputDirection = Direction;
	LastAcceptedInputTime = CurrentTime;
}

void FCombatSampling::StartBlock()
{
	if (bIsBlocking)
	{
		return;
	}

	bIsBlocking = true;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][格挡开始] 角色=%s 当前攻击状态=%d"),
		CSActor ? *CSActor->GetName() : TEXT("无"),
		static_cast<int32>(AttackState)
	);
}

void FCombatStatusSwitch::StopBlock()
{
	if (!bIsBlocking)
	{
		return;
	}

	bIsBlocking = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][格挡结束] 角色=%s 当前攻击状态=%d"),
		CSActor ? *CSActor->GetName() : TEXT("无"),
		static_cast<int32>(AttackState)
	);
}

bool FCombatStatusSwitch::IsAttackActive() const
{
	UWorld* World = CSWorld;
	if (!World)
	{
		return false;
	}

	return HasActiveAttack(World->GetTimeSeconds());

}

void FCombatStatusSwitch::StartConvertedGuard(
	EAttackDirection Direction,
	float CurrentTime,
	float GuardDuration
)
{
	const float SafeGuardDuration = FMath::Max(0.1f, GuardDuration);

	bIsBlocking = true;
	bIsDeflecting = false;

	CombatDamage.CurrentBaseDamage = 0.0f;
	CombatDamage.CurrentDamageModifier = 1.0f;
	CurrentWindowTime = SafeGuardDuration;
	CurrentAttackStartTime = CurrentTime;
	CurrentAttackEndTime = CurrentTime + SafeGuardDuration;
	CurrentDirection = Direction;
	CurrentAttackType = TEXT("Guard");
	bWeaponTraceWindowOpen = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][攻击转格挡] 角色=%s 方向=%s 开始=%.3f 时长=%.3f"),
		CSActor ? *CSActor->GetName() : TEXT("无"),
		AttackDirectionToChinese(Direction),
		CurrentAttackStartTime,
		SafeGuardDuration
	);
}

void FCombatStatusSwitch::StartDeflect()
{
	if (bIsDeflecting)
	{
		return;
	}

	bIsDeflecting = true;

	// 弹刀时不应继续保持格挡有效帧
	bIsBlocking = false;

	// 弹刀时当前攻击被打断，防止继续造成伤害
	CurrentAttackStartTime = -1.0f;
	CombatDamage.CurrentBaseDamage = 0.0f;
	CombatDamage.CurrentDamageModifier = 1.0f;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][弹刀开始] 角色=%s"),
		CSActor ? *CSActor->GetName() : TEXT("无")
	);
}

void FCombatStatusSwitch::EndDeflect()
{
	if (!bIsDeflecting)
	{
		return;
	}

	bIsDeflecting = false;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][弹刀结束] 角色=%s"),
		CSActor ? *CSActor->GetName() : TEXT("无")
	);
}

void FCombatStatusSwitch::InterruptCurrentAttack()
{
	FAttackAnimationPlayer::StopAttackMontage(CSActor, nullptr, 0.1f);

	bIsBlocking = false;
	bIsDeflecting = false;

	CurrentAttackStartTime = -1.0f;
	CurrentAttackEndTime = -1.0f;
	CurrentWindowTime = 0.0f;
	CurrentDirection = EAttackDirection::None;
	CurrentAttackType = NAME_None;
	CombatDamage.CurrentBaseDamage = 0.0f;
	bWeaponTraceWindowOpen = false;

	ClearPendingAttack();
	ClearSamplingBuffer();

	AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[攻击交互][攻击组件被打断] 角色=%s 新状态=%d"),
		CSActor ? *CSActor->GetName() : TEXT("无"),
		static_cast<int32>(AttackState)
	);
}

const FAttackMoveData* FCombatStatusSwitch::FindAttackMoveByDirection(EAttackDirection InDirection) const
{
	if (!AttackMoveDataAsset || InDirection == EAttackDirection::None)
	{
		return nullptr;
	}

	return AttackMoveDataAsset->FindAttackByDirection(InDirection);
}