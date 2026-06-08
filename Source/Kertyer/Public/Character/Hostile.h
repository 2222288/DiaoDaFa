#pragma once

#include "CoreMinimal.h"
#include "Character/Base.h"
#include "Combat/CombatTypes.h"
#include "Hostile.generated.h"

class UWidgetComponent;
class UAttackMoveDataAsset;
struct FAttackMoveData;

UCLASS()
class KERTYER_API AHostile : public ABase
{
	GENERATED_BODY()

public:
	AHostile();

	//执行攻击
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Attack();

	//是否能继续攻击
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool CanAttack() const;

	//获取当前攻击伤害
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetCurrentAttackDamage() const;

	//伤害处理函数
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;

	//血条UI组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	//血条附着的骨骼名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FName HealthBarAttachSocketName = TEXT("head");

	//血条相对于骨骼的偏移位置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FVector HealthBarOffset = FVector(0.0f, 0.0f, 30.0f);

	//攻击动作数据资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (DisplayName = "攻击动作数据资产"))
	TObjectPtr<UAttackMoveDataAsset> AttackMoveDataAsset = nullptr;

	//能否攻击
	bool bCanAttack = true;

	//是否正在攻击
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	EAttackDirection CurrentHostileAttackDirection = EAttackDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	FName CurrentHostileAttackType = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentHostileAttackStartTime = 0.0f;

	//当前攻击伤害
	float CurrentAttackDamage = 0.0f;

	//攻击开始后多久进入伤害窗口
	FTimerHandle EnableDamageTimer;

	//多少时间后关闭武器碰撞
	FTimerHandle DisableDamageTimer;

	//动画段播放结束后，结束本次攻击状态
	FTimerHandle FinishAttackTimer;

	//血条更新函数
	void UpdateHealthBar();

	//结束攻击函数
	void FinishAttack();

	//获取随机攻击动作
	const FAttackMoveData* GetRandomAttackData() const;
		
	virtual void OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason) override;
	
};