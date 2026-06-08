#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Combat/CombatTypes.h"
#include "Weapon/WeaponTypes.h"
#include "DataAsset/CombatReactionAnimationData.h"
#include "Base.generated.h"

class AWeaponBase;

/**
 * 基础角色类：
 * 负责武器持有、攻击数据下发、武器判定开关和生命值扣减。
 */
UCLASS()
class KERTYER_API ABase : public ACharacter
{
	GENERATED_BODY()

public:
	/** 构造函数：初始化基础生命值。 */
	ABase();

	/** 武器挂载到角色骨骼上的 Socket 名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器挂载Socket名称"))
	FName WeaponAttachSocketName = TEXT("hand_r_weapons");

	/** 设置当前武器，并把武器挂载到角色身上。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "设置当前武器"))
	void SetCurrentWeapon(AWeaponBase* NewWeapon);

	/** 获取当前武器。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前武器"))
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	/** 通知当前武器一次攻击已经开始，并把攻击数据下发给武器。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "通知武器攻击开始"))
	void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier,float CounterAttackValidWindow=0.5);

	/** 打开当前武器的命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开当前武器判定"))
	void EnableWeaponTrace();

	/** 关闭当前武器的命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭当前武器判定"))
	void DisableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "身体被命中后打断当前攻击"))
	void InterruptCurrentAttackByBodyHit(AActor* DamageCauser);

	/** 最大生命值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (DisplayName = "最大生命值"))
	float MaxHealth;

	/** 当前生命值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (DisplayName = "当前生命值"))
	float CurrentHealth;

	/** 治疗角色。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes", meta = (DisplayName = "治疗"))
	void Treat(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "开始弹刀"))
	void StartDeflect();

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "结束弹刀"))
	void EndDeflect();

	UFUNCTION(BlueprintPure, Category = "Combat|Deflect", meta = (DisplayName = "当前是否正在弹刀"))
	bool IsDeflecting() const { return bIsDeflecting; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	bool PlayCombatReaction(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult = EWeaponContactResult::Ignore,
		EAttackDirection Direction = EAttackDirection::None,
		bool bSelfIsSlower = false,
		bool bValidTimedResponse = false
	);

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayWeaponContactReaction(
		const FWeaponContactResolveOutput& ResolveOutput,
		EWeaponContactSide SelfSide
	);

	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayHitReaction();

	// 攻击请求阶段尝试把本次攻击转换为格挡。
// 成功返回 true，并通过 OutGuardDuration 返回格挡动画时长。
	bool TryConvertAttackToGuard(
		EAttackDirection GuardDirection,
		float GuardRequestTime,
		float& OutGuardDuration
	);

	// 播放战斗反应，并返回实际播放时长。
	bool PlayCombatReactionAndGetLength(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult,
		EAttackDirection Direction,
		bool bSelfIsSlower,
		bool bValidTimedResponse,
		float& OutPlayedLength
	);

	// 对向格挡成功后，给下一次真正攻击加速。
	void GrantNextAttackSpeedBonus(float PlayRateMultiplier);

	// 消耗下一次攻击加速倍率。
	float ConsumeNextAttackPlayRateModifier();

	// 格挡抵消后，取消对方当前攻击。
	void CancelCurrentAttackByGuard(AActor* GuardActor, const FString& Reason);

	// 扣血但不播放普通 Hit 反应；死亡仍然播放 Death。
	void ApplyDamageWithoutNonLethalHitReaction(
		float DamageAmount,
		AController* EventInstigator,
		AActor* DamageCauser
	);


	/** 接收伤害并结算当前生命值。 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;



protected:
	/** 游戏开始时生成或挂载默认武器。 */
	virtual void BeginPlay() override;

	/** 默认武器类。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "默认武器类"))
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	/** 当前持有的武器实例。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前武器"))
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction")
	TObjectPtr<UCombatReactionAnimationDataAsset> CombatReactionAnimationData = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Deflect", meta = (DisplayName = "是否正在弹刀"))
	bool bIsDeflecting = false;

	// 完美格挡后下一次攻击播放倍率。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "完美格挡后下次攻击速度倍率"))
	float PerfectGuardNextAttackSpeedMultiplier = 1.25f;

	// 攻击前自动格挡搜索半径。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "攻击前格挡检测半径"))
	float PreAttackGuardSearchRadius = 800.0f;

	// 等速判定容差。小于等于该时间差时，不算“我慢于对方”。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "等速判定容差"))
	float EqualAttackTimeTolerance = 0.12f;

	// 下一次攻击播放倍率。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Guard", meta = (DisplayName = "下一次攻击速度倍率"))
	float NextAttackPlayRateModifier = 1.0f;

	// 下次非致命受伤是否跳过 Hit 动画。
	bool bSuppressNextNonLethalHitReaction = false;

	// 当前攻击被格挡取消时的角色专用回调；Hostile 用它清理攻击状态。
	virtual void OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason);

	// 查找当前攻击前格挡需要比较的对手。
	ABase* FindPreAttackGuardOpponent() const;

};

