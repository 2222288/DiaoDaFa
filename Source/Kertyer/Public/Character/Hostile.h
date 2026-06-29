#pragma once

#include "CoreMinimal.h"
#include "Character/Base.h"
#include "Combat/CombatTypes.h"
#include "Hostile.generated.h"

class UWidgetComponent;
class UAttackMoveDataAsset;
class UAnimMontage;
struct FAttackMoveData;

UCLASS()
class KERTYER_API AHostile : public ABase
{
    GENERATED_BODY()

public:
    AHostile();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void Attack();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool CanAttack() const;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    float GetCurrentAttackDamage() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> HealthBarWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FName HealthBarAttachSocketName = TEXT("head");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FVector HealthBarOffset = FVector(0.0f, 0.0f, 30.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "攻击动作数据资产"))
    TObjectPtr<UAttackMoveDataAsset> AttackMoveDataAsset = nullptr;

    bool bCanAttack = true;
    bool bIsAttacking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    EAttackDirection CurrentHostileAttackDirection = EAttackDirection::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    FName CurrentHostileAttackType = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    float CurrentHostileAttackStartTime = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> CurrentAttackMontage = nullptr;

    // 普通攻击开始后，武器会进入自己的攻击周期；结束/中断时必须成对收尾。
    // 转换为 Guard 的情况不会调用 NotifyWeaponAttackStarted，因此这里保持 false。
    bool bWeaponAttackCycleActive = false;

    float CurrentAttackDamage = 0.0f;

    // 仅用于“攻击请求转换为 Guard 反应”的兼容结束计时；普通攻击不再使用时长计时器。
    FTimerHandle ConvertedGuardFinishTimer;

    UFUNCTION()
    void HandleHealthChanged(float OldHealth, float NewHealth, float InMaxHealth);

    void UpdateHealthBar(float HealthPercent);

    bool BindAttackMontageDelegates(UAnimMontage* Montage);

    void HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

    void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void CompleteAttackState(UAnimMontage* Montage, bool bInterrupted, const TCHAR* EventSource);

    void FinishConvertedGuard();

    const FAttackMoveData* GetRandomAttackData() const;

    virtual void CancelActiveAttack(const FString& Reason) override;

    virtual void OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason) override;
};