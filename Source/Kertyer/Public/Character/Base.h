#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DataAsset/AttackDH.h"
#include "Weapon/WeaponTypes.h"
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
	void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier);

	/** 打开当前武器的命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开当前武器判定"))
	void EnableWeaponTrace();

	/** 关闭当前武器的命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭当前武器判定"))
	void DisableWeaponTrace();

	/** 最大生命值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes", meta = (DisplayName = "最大生命值"))
	float MaxHealth;

	/** 当前生命值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (DisplayName = "当前生命值"))
	float CurrentHealth;

	/** 治疗角色。 */
	UFUNCTION(BlueprintCallable, Category = "Attributes", meta = (DisplayName = "治疗"))
	void Treat(float HealAmount);

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
};