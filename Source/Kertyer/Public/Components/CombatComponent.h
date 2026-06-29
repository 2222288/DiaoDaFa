#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatDamage.h"
#include "Combat/CombatSampling.h"
#include "Combat/CombatStatusSwitch.h"
#include "Combat/CombatTypes.h"
#include "CombatComponent.generated.h"

class UAttackMoveDataAsset;
class UAnimMontage;

/**
 * 攻击组件：接收输入并调度采样、状态和伤害模块。
 * 攻击生命周期由当前攻击蒙太奇的 Blend Out / End 委托结束，不再依赖预计播放时长。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    virtual void BeginPlay() override;

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    UFUNCTION(BlueprintCallable, Category = "Combat|Sampling", meta = (DisplayName = "开始攻击采样"))
    void BeginAttackSampling(float CurrentTime);

    UFUNCTION(BlueprintCallable, Category = "Combat|Sampling", meta = (DisplayName = "结束攻击采样"))
    void EndAttackSampling();

    void CacheMouseInput(const FVector2D& Input, float CurrentTime);

    UFUNCTION(BlueprintCallable, Category = "Combat|Block", meta = (DisplayName = "开始格挡"))
    void StartBlock();

    UFUNCTION(BlueprintCallable, Category = "Combat|Block", meta = (DisplayName = "停止格挡"))
    void StopBlock();

    UFUNCTION(BlueprintPure, Category = "Combat|Block", meta = (DisplayName = "当前是否正在格挡"))
    bool IsBlocking() const;

    UFUNCTION(BlueprintPure, Category = "Combat", meta = (DisplayName = "获取攻击状态"))
    EAttackState GetAttackState() const;

    UFUNCTION(BlueprintPure, Category = "Combat", meta = (DisplayName = "获取当前攻击方向"))
    EAttackDirection GetCurrentDirection() const;

    int32 GetAttackTriggerCounter() const;

    bool IsAttackActive() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Damage", meta = (DisplayName = "获取当前攻击伤害倍率"))
    float GetCurrentDamageModifier() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Damage", meta = (DisplayName = "获取当前攻击最终伤害"))
    float GetCurrentAttackDamage() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Damage", meta = (DisplayName = "获取当前攻击基础伤害"))
    float GetCurrentBaseDamage() const;

    FName GetCurrentAttackType() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器判定窗口"))
    void EnableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器判定窗口"))
    void DisableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Interrupt", meta = (DisplayName = "打断当前攻击"))
    void InterruptCurrentAttack();

    UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "开始弹刀"))
    void StartDeflect();

    UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "结束弹刀"))
    void EndDeflect();

    UFUNCTION(BlueprintPure, Category = "Combat|Deflect", meta = (DisplayName = "当前是否正在弹刀"))
    bool IsDeflecting() const;

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "攻击动作数据资产"))
    TObjectPtr<UAttackMoveDataAsset> AttackMoveDataAsset = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Sampling", meta = (DisplayName = "鼠标采样最小距离"))
    float MinSampleDistance = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input", meta = (DisplayName = "方向切换冷却"))
    float DirectionSwitchCooldown = 0.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input", meta = (DisplayName = "攻击请求冷却"))
    float AttackRequestCooldown = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Input", meta = (DisplayName = "默认反击有效窗口"))
    float CounterAttackValidWindow = 0.5f;

private:
    void PerformAttackAndBind(EAttackDirection Direction, float TrackScore);

    bool BindAttackMontageDelegates(UAnimMontage* Montage);

    void HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

    void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void FinalizeCurrentAttack(UAnimMontage* Montage, bool bInterrupted, const TCHAR* EventSource);

    void UpdateTickEnabled();

private:
    FCombatStatusSwitch CombatStatusSwitch;
    FCombatSampling CombatSampling;
    FCombatDamage CombatDamage;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> CurrentAttackMontage = nullptr;
};
