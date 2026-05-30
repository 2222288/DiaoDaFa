#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponBase.generated.h"

class ABase;
class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * 武器基类：
 * 负责接收角色攻击数据、打开/关闭武器判定窗口、
 * 通过刀刃 Socket Sweep 处理身体命中和武器碰撞。
 */
UCLASS()
class KERTYER_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数：初始化武器组件、碰撞和默认刀刃 Socket。 */
	AWeaponBase();

	/** 每帧执行武器 Trace。 */
	virtual void Tick(float DeltaSeconds) override;

	/** 设置当前武器持有者。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "设置当前持有者"))
	void SetCurrentHolder(ABase* NewHolder);

	/** 获取当前武器持有者。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前持有者"))
	ABase* GetCurrentHolder() const { return CurrentHolder; }

	/** 接收角色下发的本次攻击数据。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "接收攻击数据"))
	void ReceiveAttackData(const FWeaponAttackData& AttackData);

	/** 打开武器命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器判定"))
	void EnableWeaponTrace();

	/** 关闭武器命中判定窗口。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器判定"))
	void DisableWeaponTrace();

	/** 打开武器接触窗口，等价于打开武器判定。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器接触窗口"))
	void OpenWeaponContactWindow() { EnableWeaponTrace(); }

	/** 关闭武器接触窗口，等价于关闭武器判定。 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器接触窗口"))
	void CloseWeaponContactWindow() { DisableWeaponTrace(); }

	/** 当前武器是否正在进行 Trace。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "武器是否正在判定"))
	bool IsWeaponTracing() const { return bIsTracing; }

	/** 获取当前攻击方向。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前攻击方向"))
	EAttackDirection GetCurrentAttackDirection() const { return CurrentAttackDirection; }

	/** 获取当前武器状态。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取武器状态"))
	EWeaponState GetWeaponState() const { return CurrentWeaponState; }

	/** 获取武器重量。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取武器重量"))
	float GetWeaponWeight() const { return WeaponWeight; }

	/** 获取武器接触强度。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取武器接触强度"))
	float GetContactStrength() const { return WeaponContactStrength; }

	/** 获取当前攻击最终伤害。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "获取当前攻击最终伤害"))
	float GetCurrentAttackDamage() const { return CurrentAttackData.GetFinalDamage(); }

	/** 获取当前攻击数据。 */
	const FWeaponAttackData& GetCurrentAttackData() const { return CurrentAttackData; }

protected:
	/** 游戏开始时初始化武器状态。 */
	virtual void BeginPlay() override;

	/** 重置每个刀刃 Socket 的上一帧位置。 */
	void ResetSocketTracePositions();

	/** 执行刀刃 Socket Sweep。 */
	void PerformSocketSweeps(float DeltaSeconds);

	/** 处理单次 Sweep 命中结果。 */
	void ProcessSweepHit(const FHitResult& Hit);

	/** 处理武器命中角色身体。 */
	void HandleBodyHit(ABase* HitBody, const FHitResult& Hit);

	/** 处理我方武器与敌方武器碰撞。 */
	void HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit);

	/** 根据武器碰撞结果修改双方武器状态。 */
	void ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result);

	/** 蓝图事件：武器命中身体后触发。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback", meta = (DisplayName = "武器命中身体事件"))
	void BP_OnBodyHit(ABase* HitBody, const FHitResult& Hit, float Damage);

	/** 蓝图事件：武器与武器接触后触发。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback", meta = (DisplayName = "武器接触事件"))
	void BP_OnWeaponContact(AWeaponBase* OtherWeapon, EWeaponContactResult Result, const FHitResult& Hit);

public:
	/** 场景根组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "场景根组件"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** 武器显示网格。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "武器网格"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** 武器碰撞组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "武器碰撞组件"))
	TObjectPtr<UBoxComponent> WeaponCollision;

	/** 武器类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器类型"))
	EWeaponType WeaponType = EWeaponType::Sword;

	/** 武器重量，用于武器接触判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器重量"))
	float WeaponWeight = 1.0f;

	/** 武器接触强度，用于武器接触判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "武器接触强度"))
	float WeaponContactStrength = 1.0f;

	/** Socket Sweep 的球体半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace", meta = (DisplayName = "武器判定半径"))
	float TraceSphereRadius = 8.0f;

	/** 刀刃 Socket 名称列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace", meta = (DisplayName = "刀刃Socket名称列表"))
	TArray<FName> BladeSocketNames;

	/** 当前武器持有者。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前持有者"))
	TObjectPtr<ABase> CurrentHolder;

	/** 当前是否正在进行武器 Trace。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "是否正在判定"))
	bool bIsTracing = false;

	/** 当前攻击方向。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击方向"))
	EAttackDirection CurrentAttackDirection = EAttackDirection::None;

	/** 当前武器状态。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前武器状态"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;

	/** 当前攻击数据。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击数据"))
	FWeaponAttackData CurrentAttackData;

private:
	/** 每个刀刃 Socket 上一帧的位置。 */
	TMap<FName, FVector> PreviousSocketLocations;

	/** 当前判定窗口内已经命中过的身体，防止同一窗口重复扣血。 */
	TSet<TObjectPtr<ABase>> HitActorsThisTrace;

	/** 当前判定窗口内已经接触过的武器，防止重复处理同一次武器碰撞。 */
	TSet<TObjectPtr<AWeaponBase>> ContactedWeaponsThisTrace;
};