#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/CombatTypes.h"
#include "Weapon/WeaponTypes.h"
#include "Base.generated.h"

class AWeaponBase;
class UBaseHealthComponent;
class UWeaponHolderComponent;
class UCombatReactionComponent;
class UDefenseComponent;

UCLASS()
class KERTYER_API ABase : public ACharacter
{
	GENERATED_BODY()

public:
	ABase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "生命组件"))
	TObjectPtr<UBaseHealthComponent> BaseHealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "武器持有组件"))
	TObjectPtr<UWeaponHolderComponent> WeaponHolderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "战斗反应组件"))
	TObjectPtr<UCombatReactionComponent> CombatReactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "防御组件"))
	TObjectPtr<UDefenseComponent> DefenseComponent;

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "设置当前武器"))
	void SetCurrentWeapon(AWeaponBase* NewWeapon);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前武器"))
	AWeaponBase* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "通知武器攻击开始"))
	void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier, float CounterAttackValidWindow = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "通知武器攻击结束"))
	void NotifyWeaponAttackFinished(bool bInterrupted);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开当前武器判定"))
	void EnableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭当前武器判定"))
	void DisableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "身体被命中后打断当前攻击"))
	void InterruptCurrentAttackByBodyHit(AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "获取最大生命值"))
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "获取当前生命值"))
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "生命百分比"))
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Attributes", meta = (DisplayName = "是否死亡"))
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category = "Attributes", meta = (DisplayName = "治疗"))
	void Treat(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "开始弹刀"))
	void StartDeflect();

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "结束弹刀"))
	void EndDeflect();

	UFUNCTION(BlueprintPure, Category = "Combat|Deflect", meta = (DisplayName = "当前是否正在弹刀"))
	bool IsDeflecting() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	bool PlayCombatReaction(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult = EWeaponContactResult::Ignore,
		EAttackDirection Direction = EAttackDirection::None,
		bool bSelfIsSlower = false,
		bool bValidTimedResponse = false
	);

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayWeaponContactReaction(const FWeaponContactResolveOutput& ResolveOutput, EWeaponContactSide SelfSide);

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayHitReaction();

	bool TryConvertAttackToGuard(EAttackDirection GuardDirection, float GuardRequestTime, float& OutGuardDuration);

	bool PlayCombatReactionAndGetLength(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult,
		EAttackDirection Direction,
		bool bSelfIsSlower,
		bool bValidTimedResponse,
		float& OutPlayedLength
	);

	void GrantNextAttackSpeedBonus(float PlayRateMultiplier);
	float ConsumeNextAttackPlayRateModifier();

	void CancelCurrentAttackByGuard(AActor* GuardActor, const FString& Reason);

	void ApplyDamageWithoutNonLethalHitReaction(float DamageAmount, AController* EventInstigator, AActor* DamageCauser);

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

	/** 子类取消自身特有的攻击状态；AHostile 在此只停止当前攻击蒙太奇。 */
	virtual void CancelActiveAttack(const FString& Reason);

	virtual void OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason);

	ABase* FindPreAttackGuardOpponent() const;
};