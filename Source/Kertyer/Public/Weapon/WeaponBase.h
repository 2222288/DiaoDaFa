#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponBase.generated.h"

class ABase;
class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class KERTYER_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	virtual void Tick(float DeltaSeconds) override;

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

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "武器是否正在判定"))
	bool IsWeaponTracing() const { return bIsTracing; }

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

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "强制停止武器交互"))
	void ForceStopWeaponInteraction(const FString& Reason);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "本次窗口是否已完成交互"))
	bool HasResolvedInteractionThisTrace() const { return bHasResolvedInteractionThisTrace; }

protected:
	virtual void BeginPlay() override;

	void ResetSocketTracePositions();

	void PerformSocketSweeps(float DeltaSeconds);

	void ProcessSweepHit(const FHitResult& Hit);

	void HandleBodyHit(ABase* HitBody, const FHitResult& Hit);

	void HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit);

	void ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result);

	bool ShouldIgnoreBodyHitByCounterWindow(ABase* HitBody, const FHitResult& Hit, float& OutElapsed, float& OutWindow, FString& OutReason) const;

	void ApplyBodyDamageAndInterrupt(ABase* TargetBody, AWeaponBase* DamageSourceWeapon, const FHitResult& Hit, float Damage, const FString& Reason);

	bool HasConsumedInteraction() const;

	void MarkInteractionConsumed(AActor* ConsumedActor);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前持有者"))
	TObjectPtr<ABase> CurrentHolder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "是否正在判定"))
	bool bIsTracing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击方向"))
	EAttackDirection CurrentAttackDirection = EAttackDirection::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前武器状态"))
	EWeaponState CurrentWeaponState = EWeaponState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "当前攻击数据"))
	FWeaponAttackData CurrentAttackData;

private:
	TMap<FName, FVector> PreviousSocketLocations;

	TSet<TObjectPtr<ABase>> HitActorsThisTrace;

	TSet<TObjectPtr<AWeaponBase>> ContactedWeaponsThisTrace;

	bool bHasResolvedInteractionThisTrace = false;

	TWeakObjectPtr<AActor> ConsumedActorThisTrace;

	TSet<TWeakObjectPtr<ABase>> IgnoredBodyActorsThisTraceForLog;
};