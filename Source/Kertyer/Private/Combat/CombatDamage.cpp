#include "Combat/CombatDamage.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Character/Base.h"
#include "Combat/CombatSampling.h"
#include "Combat/CombatStatusSwitch.h"
#include "DataAsset/AttackMoveDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
    FString SafeActorName(const AActor* Actor)
    {
        return IsValid(Actor) ? Actor->GetName() : FString(TEXT("无"));
    }
}

void FCombatDamage::Initialize()
{
    CurrentDamageModifier = 1.0f;
    NextAttackDamageModifier = 1.0f;
    CurrentBaseDamage = 0.0f;
    CurrentAttackType = NAME_None;
    bWeaponTraceWindowOpen = false;
}

void FCombatDamage::EnableWeaponTrace(AActor* Owner)
{
    if (bWeaponTraceWindowOpen)
    {
        return;
    }

    if (ABase* OwnerCharacter = Cast<ABase>(Owner))
    {
        bWeaponTraceWindowOpen = true;
        OwnerCharacter->EnableWeaponTrace();
    }
}

void FCombatDamage::DisableWeaponTrace(AActor* Owner)
{
    if (!bWeaponTraceWindowOpen)
    {
        return;
    }

    if (ABase* OwnerCharacter = Cast<ABase>(Owner))
    {
        bWeaponTraceWindowOpen = false;
        OwnerCharacter->DisableWeaponTrace();
    }
}

void FCombatDamage::ForceDisableWeaponTrace(AActor* Owner)
{
    bWeaponTraceWindowOpen = false;
    if (ABase* OwnerCharacter = Cast<ABase>(Owner))
    {
        OwnerCharacter->DisableWeaponTrace();
    }
}

UAnimMontage* FCombatDamage::PerformAttack(
    AActor* Owner,
    UWorld* World,
    const UAttackMoveDataAsset* AttackMoveDataAsset,
    FCombatStatusSwitch& Status,
    FCombatSampling& Sampling,
    EAttackDirection Direction,
    float TrackScore
)
{
    ABase* OwnerBase = Cast<ABase>(Owner);

    if (OwnerBase && OwnerBase->IsDeflecting())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[攻击交互][攻击失败] 当前正在弹刀，不能攻击 角色=%s"),
            *SafeActorName(Owner));
        return nullptr;
    }

    if (Direction == EAttackDirection::None || !World)
    {
        return nullptr;
    }

    const float CurrentTime = World->GetTimeSeconds();
    Status.RefreshAttackState(CurrentTime);

    if (Status.IsLockedState() && Direction == Status.CurrentDirection)
    {
        Sampling.ClearSamplingBuffer();
        return nullptr;
    }

    if (Status.IsLockedState())
    {
        Sampling.QueuePendingAttack(Direction, TrackScore);
        Sampling.ClearSamplingBuffer();
        return nullptr;
    }

    const FAttackMoveData* AttackData = Sampling.FindAttackMoveByDirection(AttackMoveDataAsset, Direction);
    if (!AttackData || !AttackData->AttackMontage)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[攻击交互][出招失败] 未找到攻击动作数据或 Montage 为空 角色=%s 方向=%d"),
            *SafeActorName(Owner), static_cast<int32>(Direction));
        return nullptr;
    }

    float GuardDuration = 0.0f;
    if (OwnerBase && OwnerBase->TryConvertAttackToGuard(Direction, CurrentTime, GuardDuration))
    {
        Status.StartConvertedGuard(Owner, Direction, CurrentTime, GuardDuration);
        ResetForGuard();
        Sampling.ClearPendingAttack();
        Sampling.ClearSamplingBuffer();
        Status.RefreshAttackState(CurrentTime);
        return nullptr;
    }

    const float AttackPlayRate = OwnerBase
        ? OwnerBase->ConsumeNextAttackPlayRateModifier()
        : 1.0f;

    const FAttackAnimationPlayResult AnimationResult =
        FAttackAnimationPlayer::PlayAttackMontage(Owner, *AttackData, AttackPlayRate);

    if (!AnimationResult.bSucceeded)
    {
        if (OwnerBase && AttackPlayRate > 1.0f)
        {
            OwnerBase->GrantNextAttackSpeedBonus(AttackPlayRate);
        }

        UE_LOG(LogTemp, Error,
            TEXT("[攻击交互][出招失败] 原因=%s 角色=%s Montage=%s Section=%s 方向=%d"),
            *AnimationResult.ErrorMessage,
            *SafeActorName(Owner),
            *GetNameSafe(AttackData->AttackMontage.Get()),
            *AttackData->MontageSection.ToString(),
            static_cast<int32>(Direction));
        return nullptr;
    }

    CurrentBaseDamage = AttackData->Damage;
    CurrentDamageModifier = NextAttackDamageModifier;
    NextAttackDamageModifier = TrackScore;
    CurrentAttackType = AttackData->AttackID;
    bWeaponTraceWindowOpen = false;

    Status.MarkAttackMontageStarted(Direction, CurrentTime, AttackData->WindowTime);
    Status.AttackTriggerCounter++;

    const float ActualCounterAttackValidWindow =
        AttackData->CounterAttackValidWindow > 0.0f
        ? AttackData->CounterAttackValidWindow
        : Status.CounterAttackValidWindow;

    if (OwnerBase)
    {
        OwnerBase->NotifyWeaponAttackStarted(
            Direction,
            CurrentAttackType,
            Status.CurrentAttackStartTime,
            CurrentBaseDamage,
            CurrentDamageModifier,
            ActualCounterAttackValidWindow
        );
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 最终伤害=%.2f 播放倍率=%.3f 报告时长=%.3f 触发计数=%d"),
        *SafeActorName(Owner),
        static_cast<int32>(Direction),
        *CurrentAttackType.ToString(),
        *GetNameSafe(AnimationResult.PlayedMontage),
        *AnimationResult.PlayedSection.ToString(),
        GetCurrentAttackDamage(),
        AttackPlayRate,
        AnimationResult.PlayedLength,
        Status.AttackTriggerCounter);

    Sampling.ClearPendingAttack();
    Sampling.ClearSamplingBuffer();
    Status.RefreshAttackState(CurrentTime);

    return AnimationResult.PlayedMontage;
}

void FCombatDamage::ResetForGuard()
{
    CurrentBaseDamage = 0.0f;
    CurrentDamageModifier = 1.0f;
    CurrentAttackType = FName(TEXT("Guard"));
    bWeaponTraceWindowOpen = false;
}

void FCombatDamage::ResetAfterAttack()
{
    CurrentBaseDamage = 0.0f;
    CurrentDamageModifier = 1.0f;
    CurrentAttackType = NAME_None;
    bWeaponTraceWindowOpen = false;
}

void FCombatDamage::ResetAfterInterrupt()
{
    ResetAfterAttack();
}

void FCombatDamage::ResetForDeflect()
{
    ResetAfterAttack();
}
