#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"

class FCombatSampling;
class FCombatStatusSwitch;
class UAttackMoveDataAsset;
class UAnimMontage;

/** 战斗伤害模块：负责出招、当前攻击伤害数据和武器判定窗口。 */
class KERTYER_API FCombatDamage
{
public:
    void Initialize();

    float GetCurrentDamageModifier() const { return CurrentDamageModifier; }

    float GetCurrentAttackDamage() const { return CurrentBaseDamage * CurrentDamageModifier; }

    float GetCurrentBaseDamage() const { return CurrentBaseDamage; }

    FName GetCurrentAttackType() const { return CurrentAttackType; }

    bool IsWeaponTraceWindowOpen() const { return bWeaponTraceWindowOpen; }

    void EnableWeaponTrace(AActor* Owner);

    void DisableWeaponTrace(AActor* Owner);

    void ForceDisableWeaponTrace(AActor* Owner);

    /** 成功播放攻击时返回本次攻击蒙太奇，否则返回 nullptr。 */
    UAnimMontage* PerformAttack(
        AActor* Owner,
        UWorld* World,
        const UAttackMoveDataAsset* AttackMoveDataAsset,
        FCombatStatusSwitch& Status,
        FCombatSampling& Sampling,
        EAttackDirection Direction,
        float TrackScore
    );

    void ResetForGuard();

    void ResetAfterAttack();

    void ResetAfterInterrupt();

    void ResetForDeflect();

private:
    float CurrentDamageModifier = 1.0f;

    float NextAttackDamageModifier = 1.0f;

    float CurrentBaseDamage = 0.0f;

    FName CurrentAttackType = NAME_None;

    bool bWeaponTraceWindowOpen = false;
};
