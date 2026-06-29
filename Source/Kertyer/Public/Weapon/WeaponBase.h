#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponBase.generated.h"

class ABase;
class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UWeaponTraceComponent;
class UWeaponInteractionComponent;

UCLASS()
class KERTYER_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "设置当前持有者"))
	void SetCurrentHolder(ABase* NewHolder);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前持有者"))
	ABase* GetCurrentHolder() const { return CurrentHolder; }

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "接收攻击数据"))
	void ReceiveAttackData(const FWeaponAttackData& AttackData);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器判定"))
	void EnableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器判定"))
	void DisableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器接触窗口"))
	void OpenWeaponContactWindow() { EnableWeaponTrace(); }

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器接触窗口"))
	void CloseWeaponContactWindow() { DisableWeaponTrace(); }

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "强制停止武器交互"))
	void ForceStopWeaponInteraction(const FString& Reason);

	/** 蒙太奇的统一结束出口。负责将 Recovering/Disabled 明确恢复到 Idle。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "完成武器攻击周期"))
	void CompleteAttackCycle(bool bInterrupted);

	/**
	 * 武器状态唯一写入口。
	 * 非法转换会被拒绝并输出警告，避免多个组件直接改写状态。
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "尝试切换武器状态"))
	bool TryTransitionWeaponState(EWeaponState NewState, FName Context = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "武器是否正在判定"))
	bool IsWeaponTracing() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前攻击方向"))
	EAttackDirection GetCurrentAttackDirection() const { return CurrentAttackDirection; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取武器状态"))
	EWeaponState GetWeaponState() const { return CurrentWeaponState; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取武器重量"))
	float GetWeaponWeight() const { return WeaponWeight; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取接触强度"))
	float GetContactStrength() const { return WeaponContactStrength; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前攻击伤害"))
	float GetCurrentAttackDamage() const { return CurrentAttackData.GetFinalDamage(); }

	const FWeaponAttackData& GetCurrentAttackData() const { return CurrentAttackData; }

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "本次窗口是否已完成交互"))
	bool HasResolvedInteractionThisTrace() const;

	UWeaponTraceComponent* GetWeaponTraceComponent() const { return WeaponTraceComponent.Get(); }
	UWeaponInteractionComponent* GetWeaponInteractionComponent() const { return WeaponInteractionComponent.Get(); }

	ECollisionChannel GetWeaponObjectChannel() const
	{
		return WeaponObjectChannel.GetValue();
	}

	const TArray<TEnumAsByte<ECollisionChannel>>& GetWeaponTraceObjectChannels() const
	{
		return WeaponTraceObjectChannels;
	}

	void DispatchBodyHitFeedback(ABase* HitBody, const FHitResult& Hit, float Damage);
	void DispatchWeaponContactFeedback(
		AWeaponBase* OtherWeapon,
		EWeaponContactResult Result,
		const FHitResult& Hit
	);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback", meta = (DisplayName = "武器命中身体反馈"))
	void BP_OnBodyHit(ABase* HitBody, const FHitResult& Hit, float Damage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback", meta = (DisplayName = "武器碰撞反馈"))
	void BP_OnWeaponContact(AWeaponBase* OtherWeapon, EWeaponContactResult Result, const FHitResult& Hit);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "场景根组件"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "武器网格"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "武器碰撞组件"))
	TObjectPtr<UBoxComponent> WeaponCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器类型"))
	EWeaponType WeaponType = EWeaponType::Sword;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器重量"))
	float WeaponWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器接触强度"))
	float WeaponContactStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace", meta = (DisplayName = "武器判定半径"))
	float TraceSphereRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace", meta = (DisplayName = "刀刃Socket名称列表"))
	TArray<FName> BladeSocketNames;

	/**
	 * 建议在 Project Settings > Collision 中新建 Weapon Object Channel，
	 * 然后在武器蓝图中把本字段改为该通道。
	 * 默认 WorldDynamic 仅用于兼容旧资产。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Collision", meta = (DisplayName = "武器对象通道"))
	TEnumAsByte<ECollisionChannel> WeaponObjectChannel = ECC_WorldDynamic;

	/**
	 * 武器轨迹允许查询的目标对象通道。
	 * 建议配置 Damageable、Breakable 等专用 Object Channel；武器通道会自动加入查询。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Collision", meta = (DisplayName = "武器轨迹目标对象通道"))
	TArray<TEnumAsByte<ECollisionChannel>> WeaponTraceObjectChannels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前持有者"))
	TObjectPtr<ABase> CurrentHolder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击方向"))
	EAttackDirection CurrentAttackDirection = EAttackDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前武器状态"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击数据"))
	FWeaponAttackData CurrentAttackData;

private:
	bool IsLegalStateTransition(EWeaponState FromState, EWeaponState ToState) const;
	void PrepareForNewAttack();
	void ConfigureWeaponCollision();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponTraceComponent> WeaponTraceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWeaponInteractionComponent> WeaponInteractionComponent;
};
