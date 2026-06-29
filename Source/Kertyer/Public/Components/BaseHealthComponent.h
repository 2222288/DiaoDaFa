#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnHealthChangedSignature,
	float, OldHealth,
	float, NewHealth,
	float, MaxHealth
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnHealedSignature,
	float, ActualHealAmount,
	float, OldHealth,
	float, NewHealth
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

/**
 * 基础生命组件：生命值的唯一数据源。
 * 所有运行时血量变化必须通过 Treat、ApplyDamage、SetMaxHealth 或 ResetHealth 完成，
 * 以确保数值、死亡状态和 UI 事件始终一致。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UBaseHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaseHealthComponent();

	/** 任意有效血量变化后触发，包括伤害、治疗、重置和最大生命值调整。 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events", meta = (DisplayName = "生命值变化"))
	FOnHealthChangedSignature OnHealthChanged;

	/** 实际发生治疗后触发；满血或非法治疗量不会触发。 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events", meta = (DisplayName = "受到治疗"))
	FOnHealedSignature OnHealed;

	/** 生命值首次由大于 0 变为 0 时触发。 */
	UPROPERTY(BlueprintAssignable, Category = "Health|Events", meta = (DisplayName = "死亡"))
	FOnDeathSignature OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Health", meta = (DisplayName = "治疗"))
	void Treat(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Health", meta = (DisplayName = "应用伤害"))
	float ApplyDamage(float DamageAmount, float& OutOldHealth, float& OutActualDamage);

	UFUNCTION(BlueprintCallable, Category = "Health", meta = (DisplayName = "设置最大生命值"))
	void SetMaxHealth(float NewMaxHealth, bool bKeepHealthPercent = false);

	UFUNCTION(BlueprintCallable, Category = "Health", meta = (DisplayName = "重置生命值"))
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category = "Health", meta = (DisplayName = "获取最大生命值"))
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health", meta = (DisplayName = "获取当前生命值"))
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health", meta = (DisplayName = "是否死亡"))
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Health", meta = (DisplayName = "生命百分比"))
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Health", meta = (DisplayName = "抑制下次非致命受击反应"))
	void SuppressNextNonLethalHitReaction();

	bool ConsumeNextNonLethalHitReactionSuppressed();

protected:
	virtual void BeginPlay() override;

private:
	/** 仅允许在组件详情面板配置；运行时修改必须调用 SetMaxHealth。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true", DisplayName = "最大生命值", ClampMin = "0.0"))
	float MaxHealth = 100.0f;

	/** 运行时只读，禁止蓝图或外部 C++ 绕过事件直接赋值。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true", DisplayName = "当前生命值"))
	float CurrentHealth = 100.0f;

	bool bSuppressNextNonLethalHitReaction = false;

	void BroadcastHealthChanged(float OldHealth);
};
