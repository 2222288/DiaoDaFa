// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOn.generated.h"

class ACharacter;
class APlayerController;
class APawn;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API ULockOn : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOn();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	//DeltaTime距离上一秒更新的时间
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 角色输入调用
	// 
	//开关锁定
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void ToggleLockOn();

	//切换左目标
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SwitchTargetLeft();

	//切换右目标
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SwitchTargetRight();

	//是否锁定
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool IsLockedOn() const { return bLockedOn; }

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	AActor* GetLockTarget() const { return LockTarget.Get(); }

	//胸口锁点世界坐标
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	FVector GetLockPointWorld() const;

	//给UI圆点/血条用
	UFUNCTION(BlueprintCallable, Category = "LockOn")
	bool GetLockPointScreen(FVector2D& OutScreenPos) const;

protected:
	// 钩子

	// 能否切换目标
	UFUNCTION(BlueprintNativeEvent, Category = "LockOn|Combat")
	bool CanSwitchLockTarget() const;
	virtual bool CanSwitchLockTarget_Implementation() const;

	//目标是否活着
	UFUNCTION(BlueprintNativeEvent, Category = "LockOn|Combat")
	bool IsTargetAlive(AActor* Target) const;
	virtual bool IsTargetAlive_Implementation(AActor* Target) const;

	//锁定目标是否合法
	UFUNCTION(BlueprintNativeEvent, Category = "LockOn|Combat")
	bool IsValidLockCandidate(AActor* Candidate) const;
	virtual bool IsValidLockCandidate_Implementation(AActor* Candidate) const;

private:

	//获取锁定目标
	bool AcquireTarget();
	//被解除锁定逻辑
	void ClearTarget();
	//设置锁定目标
	void SetTarget(AActor* NewTarget);
	//每帧对准目标（Yaw+Pitch）
	void UpdateLockOn(float DeltaSeconds);
	//判断当前锁定是否合法 
	bool ValidateLockedTarget(float DeltaSeconds);
	//切换目标逻辑
	void SwitchTarget(int32 Dir);

	//目标胸口锁点
	FVector GetTargetLockPoint(AActor* Target) const;
	//目标是否处于预定角度内
	bool IsInKeepAngle(AActor* Target) const;
	//目标是否可见
	bool IsVisibleToTarget(AActor* Target) const;
	float ScoreForAcquire(AActor* Candidate, const FVector2D& ScreenCenter, float ScreenDiag) const;
	//获取视口信息（中心点/对角线长度/宽高）
	bool GetViewportInfo(FVector2D& OutCenter, float& OutDiag, int32& OutW, int32& OutH) const;

	void ApplyLockOnCursorMode(bool bEnable);

	FName CachedTargetSocket = NAME_None;

	//解除对象的锁定
	UFUNCTION()
	void OnTargetDestroyed(AActor* DestroyedActor);

private:
	//可调参数

	//锁定距离
	UPROPERTY(EditAnywhere, Category = "LockOn|Range")
	float MaxDistanceCm = 10000.0f;

	//锁定角度
	UPROPERTY(EditAnywhere, Category = "LockOn|Angle")
	float KeepHalfAngleDeg = 0.059f;

	//遮挡最大时间
	UPROPERTY(EditAnywhere, Category = "LockOn|Occlusion")
	float OcclusionBreakSeconds = 1.5f;

	//相机吸附速度
	UPROPERTY(EditAnywhere, Category = "LockOn|Rotation")
	float RotationInterpSpeed = 12.0f;

	//俯仰最小
	UPROPERTY(EditAnywhere, Category = "LockOn|Rotation")
	float MinPitchDeg = -80.0f;

	//俯仰最大
	UPROPERTY(EditAnywhere, Category = "LockOn|Rotation")
	float MaxPitchDeg = 80.0f;

	//可见性检测通道
	UPROPERTY(EditAnywhere, Category = "LockOn|Trace")
	TEnumAsByte<ECollisionChannel> VisibilityChannel = ECC_Visibility;

	//目标胸口节点
	UPROPERTY(EditAnywhere, Category = "LockOn|LockPoint")
	TArray<FName> ChestSocketCandidates = { TEXT("spine_03"), TEXT("spine_02"), TEXT("chest") };

	//没有胸口节点的大概位置
	UPROPERTY(EditAnywhere, Category = "LockOn|LockPoint")
	float FallbackChestZOffset = 60.0f;

	//准星权重
	UPROPERTY(EditAnywhere, Category = "LockOn|Acquire")
	float WeightCrosshair = 0.25f;

	//距离权重
	UPROPERTY(EditAnywhere, Category = "LockOn|Acquire")
	float WeightDistance = 0.75f;

	//切换的最小像素距离
	UPROPERTY(EditAnywhere, Category = "LockOn|Switch")
	float MinSwitchPixelDelta = 8.0f;

	//切换目标冷却最大值
	UPROPERTY(EditAnywhere, Category = "LockOn|Switch")
	float SwitchCooldownSeconds = 0.18f;

	UPROPERTY(EditAnywhere, Category = "LockOn|Trace")
	float VisibilityCheckInterval = 0.15f;

	float VisibilityCheckTimer = 0.0f;
	bool bLastKnownVisibility = false;


private:
	//运行时状态

	//当前角色
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	//当前控制器
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> OwnerPC = nullptr;

	//当前已锁定的目标
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockTarget;

	//是否锁定中
	bool bLockedOn = false;
	//遮挡累积时间
	float OccludedAccum = 0.0f;
	//切换目标冷却基本值
	float SwitchCooldown = 0.0f;
};