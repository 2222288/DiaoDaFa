#include "Combat/CombatStatusSwitch.h"

#include "Combat/CombatSampling.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

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

    FString SafeActorName(const AActor* Actor)
    {
        return IsValid(Actor) ? Actor->GetName() : FString(TEXT("无"));
    }
}

void FCombatStatusSwitch::Initialize()
{
    bIsAttackKeyDown = false;
    CurrentWindowTime = 0.0f;
    bIsBlocking = false;
    bIsDeflecting = false;
    bAttackMontageActive = false;
    bConvertedGuardActive = false;
    CurrentAttackStartTime = -1.0f;
    ConvertedGuardEndTime = -1.0f;
    CurrentDirection = EAttackDirection::None;
    AttackState = EAttackState::Idle;
    ResetAcceptedInput();
    AttackTriggerCounter = 0;
}

void FCombatStatusSwitch::RefreshAttackState(float CurrentTime)
{
    if (bConvertedGuardActive && CurrentTime >= ConvertedGuardEndTime)
    {
        bConvertedGuardActive = false;
        bIsBlocking = false;
        ConvertedGuardEndTime = -1.0f;
    }

    if (!HasActiveAttack(CurrentTime))
    {
        CurrentAttackStartTime = -1.0f;
        CurrentWindowTime = 0.0f;
        CurrentDirection = EAttackDirection::None;
        AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
        return;
    }

    // 攻击转格挡不开放连段窗口；它仍使用 Guard 动画返回时长进行兼容计时。
    if (bConvertedGuardActive && !bAttackMontageActive)
    {
        AttackState = bIsAttackKeyDown
            ? EAttackState::SamplingLocked
            : EAttackState::AttackingLocked;
        return;
    }

    const float Window = FMath::Max(0.0f, CurrentWindowTime);
    const bool bWindowOpen = CurrentTime >= CurrentAttackStartTime + Window;

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
    return bAttackMontageActive
        || (bConvertedGuardActive && ConvertedGuardEndTime > 0.0f && CurrentTime < ConvertedGuardEndTime);
}

bool FCombatStatusSwitch::IsAttackActive(const UWorld* World) const
{
    return World && HasActiveAttack(World->GetTimeSeconds());
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

bool FCombatStatusSwitch::CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const
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

    return Elapsed >= DirectionSwitchCooldown && Elapsed >= AttackRequestCooldown;
}

void FCombatStatusSwitch::MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime)
{
    LastAcceptedInputDirection = Direction;
    LastAcceptedInputTime = CurrentTime;
}

void FCombatStatusSwitch::ResetAcceptedInput()
{
    LastAcceptedInputDirection = EAttackDirection::None;
    LastAcceptedInputTime = -10000.0f;
}

void FCombatStatusSwitch::StartBlock(AActor* Owner)
{
    if (bIsBlocking)
    {
        return;
    }

    bIsBlocking = true;
    UE_LOG(LogTemp, Warning, TEXT("[攻击交互][格挡开始] 角色=%s 当前攻击状态=%d"),
        *SafeActorName(Owner), static_cast<int32>(AttackState));
}

void FCombatStatusSwitch::StopBlock(AActor* Owner)
{
    if (!bIsBlocking)
    {
        return;
    }

    bIsBlocking = false;
    UE_LOG(LogTemp, Warning, TEXT("[攻击交互][格挡结束] 角色=%s 当前攻击状态=%d"),
        *SafeActorName(Owner), static_cast<int32>(AttackState));
}

void FCombatStatusSwitch::StartConvertedGuard(
    AActor* Owner,
    EAttackDirection Direction,
    float CurrentTime,
    float GuardDuration
)
{
    const float SafeGuardDuration = FMath::Max(0.1f, GuardDuration);

    bIsBlocking = true;
    bIsDeflecting = false;
    bAttackMontageActive = false;
    bConvertedGuardActive = true;
    CurrentWindowTime = SafeGuardDuration;
    CurrentAttackStartTime = CurrentTime;
    ConvertedGuardEndTime = CurrentTime + SafeGuardDuration;
    CurrentDirection = Direction;

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][攻击转格挡] 角色=%s 方向=%s 开始=%.3f 时长=%.3f"),
        *SafeActorName(Owner), AttackDirectionToChinese(Direction), CurrentAttackStartTime, SafeGuardDuration);
}

void FCombatStatusSwitch::MarkAttackMontageStarted(
    EAttackDirection Direction,
    float CurrentTime,
    float ComboWindowTime
)
{
    bIsBlocking = false;
    bIsDeflecting = false;
    bAttackMontageActive = true;
    bConvertedGuardActive = false;
    ConvertedGuardEndTime = -1.0f;
    CurrentDirection = Direction;
    CurrentAttackStartTime = CurrentTime;
    CurrentWindowTime = FMath::Max(0.0f, ComboWindowTime);
}

void FCombatStatusSwitch::CompleteCurrentAttack(
    AActor* Owner,
    FCombatSampling& Sampling,
    bool bInterrupted
)
{
    bAttackMontageActive = false;
    bConvertedGuardActive = false;
    bIsBlocking = false;
    bIsDeflecting = false;
    CurrentAttackStartTime = -1.0f;
    ConvertedGuardEndTime = -1.0f;
    CurrentWindowTime = 0.0f;
    CurrentDirection = EAttackDirection::None;

    if (bInterrupted)
    {
        Sampling.ClearPendingAttack();
    }
    Sampling.ClearSamplingBuffer();

    AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][攻击状态清理] 角色=%s 是否中断=%s 新状态=%d"),
        *SafeActorName(Owner), bInterrupted ? TEXT("是") : TEXT("否"), static_cast<int32>(AttackState));
}

void FCombatStatusSwitch::InterruptCurrentAttack(AActor* Owner, FCombatSampling& Sampling)
{
    CompleteCurrentAttack(Owner, Sampling, true);
}

void FCombatStatusSwitch::StartDeflect(AActor* Owner)
{
    if (bIsDeflecting)
    {
        return;
    }

    bIsDeflecting = true;
    bIsBlocking = false;
    bAttackMontageActive = false;
    bConvertedGuardActive = false;
    CurrentAttackStartTime = -1.0f;
    ConvertedGuardEndTime = -1.0f;
    CurrentDirection = EAttackDirection::None;

    UE_LOG(LogTemp, Warning, TEXT("[攻击交互][弹刀开始] 角色=%s"), *SafeActorName(Owner));
}

void FCombatStatusSwitch::EndDeflect(AActor* Owner)
{
    if (!bIsDeflecting)
    {
        return;
    }

    bIsDeflecting = false;
    UE_LOG(LogTemp, Warning, TEXT("[攻击交互][弹刀结束] 角色=%s"), *SafeActorName(Owner));
}
