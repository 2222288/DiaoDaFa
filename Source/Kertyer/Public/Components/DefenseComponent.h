#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatTypes.h"
#include "Weapon/WeaponTypes.h"
#include "DefenseComponent.generated.h"

class ABase;

/** 防御组件：负责弹刀状态、攻击前转格挡、对向判定和下次攻击加速倍率。 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UDefenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDefenseComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Deflect", meta = (DisplayName = "是否正在弹刀"))
	bool bIsDeflecting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "完美格挡后下次攻击速度倍率"))
	float PerfectGuardNextAttackSpeedMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "攻击前格挡检测半径"))
	float PreAttackGuardSearchRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Guard", meta = (DisplayName = "等速判定容差"))
	float EqualAttackTimeTolerance = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Guard", meta = (DisplayName = "下一次攻击速度倍率"))
	float NextAttackPlayRateModifier = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "开始弹刀"))
	void StartDeflect();

	UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "结束弹刀"))
	void EndDeflect();

	UFUNCTION(BlueprintPure, Category = "Combat|Deflect", meta = (DisplayName = "当前是否正在弹刀"))
	bool IsDeflecting() const { return bIsDeflecting; }

	bool TryConvertAttackToGuard(EAttackDirection GuardDirection, float GuardRequestTime, float& OutGuardDuration);
	ABase* FindPreAttackGuardOpponent() const;
	void GrantNextAttackSpeedBonus(float PlayRateMultiplier);
	float ConsumeNextAttackPlayRateModifier();

private:
	static bool IsWeaponInActiveAttackState(EWeaponState State);
};