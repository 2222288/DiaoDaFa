#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponHolderComponent.generated.h"

class AWeaponBase;

/** 武器持有组件：负责默认武器生成、武器挂载和武器交互转发。 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UWeaponHolderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponHolderComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "默认武器类"))
	TSubclassOf<AWeaponBase> DefaultWeaponClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前武器"))
	TObjectPtr<AWeaponBase> CurrentWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器挂载Socket名称"))
	FName WeaponAttachSocketName = TEXT("hand_r_weapons");

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "生成默认武器"))
	void SpawnDefaultWeapon();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "设置当前武器"))
	void SetCurrentWeapon(AWeaponBase* NewWeapon);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前武器"))
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "通知武器攻击开始"))
	void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier, float CounterAttackValidWindow = 0.5f);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "通知武器攻击结束"))
	void NotifyWeaponAttackFinished(bool bInterrupted);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开当前武器判定"))
	void EnableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭当前武器判定"))
	void DisableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "强制停止武器交互"))
	void ForceStopWeaponInteraction(const FString& Reason);
};