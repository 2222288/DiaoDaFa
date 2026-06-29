#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"

class FCombatSampling;

/**
 * 战斗状态切换器：
 * 攻击是否仍然有效由蒙太奇回调驱动；时间只用于输入连段窗口和“攻击转格挡”的兼容计时。
 */
class KERTYER_API FCombatStatusSwitch
{
public:
    void Initialize();

    void RefreshAttackState(float CurrentTime);

    bool HasActiveAttack(float CurrentTime) const;

    bool IsAttackActive(const UWorld* World) const;

    bool IsSamplingState() const;

    bool IsLockedState() const;

    bool CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const;

    void MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime);

    void ResetAcceptedInput();

    void StartBlock(AActor* Owner);

    void StopBlock(AActor* Owner);

    bool IsBlocking() const { return bIsBlocking; }

    void StartConvertedGuard(
        AActor* Owner,
        EAttackDirection Direction,
        float CurrentTime,
        float GuardDuration
    );

    void MarkAttackMontageStarted(
        EAttackDirection Direction,
        float CurrentTime,
        float ComboWindowTime
    );

    void CompleteCurrentAttack(
        AActor* Owner,
        FCombatSampling& Sampling,
        bool bInterrupted
    );

    void InterruptCurrentAttack(AActor* Owner, FCombatSampling& Sampling);

    void StartDeflect(AActor* Owner);

    void EndDeflect(AActor* Owner);

    bool IsDeflecting() const { return bIsDeflecting; }

    EAttackState GetAttackState() const { return AttackState; }

    EAttackDirection GetCurrentDirection() const { return CurrentDirection; }

    int32 GetAttackTriggerCounter() const { return AttackTriggerCounter; }

public:
    bool bIsAttackKeyDown = false;

    float CurrentWindowTime = 0.0f;

    bool bIsBlocking = false;

    bool bIsDeflecting = false;

    bool bAttackMontageActive = false;

    bool bConvertedGuardActive = false;

    float CurrentAttackStartTime = -1.0f;

    float ConvertedGuardEndTime = -1.0f;

    EAttackDirection CurrentDirection = EAttackDirection::None;

    EAttackState AttackState = EAttackState::Idle;

    EAttackDirection LastAcceptedInputDirection = EAttackDirection::None;

    float LastAcceptedInputTime = -10000.0f;

    int32 AttackTriggerCounter = 0;

    float DirectionSwitchCooldown = 0.25f;

    float AttackRequestCooldown = 0.12f;

    float CounterAttackValidWindow = 0.5f;
};
