#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAsset/AttackDH.h"
#include "Attackif/Attackif.h"
#include "Attackif/AttackValid.h"
#include "AttackComponent.generated.h"

class UAnimInstance;
class UDataTable;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UAttackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UAttackComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	// 采样输入
	void CacheMouseInput(const FVector2D& Input, float CurrentTime);
	// 按下攻击键后初始化
	void BeginAttackSampling(float CurrentTime);
	// 松开攻击键后初始化
	void EndAttackSampling();
	// 开始格挡
	void StartBlock();
	// 停止格挡
	void StopBlock();
public:
	// 最小采样阈值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", DisplayName = "鼠标移动最小阈值")
	float MinSampleDistance = 8.0f;
	// 攻击数据表
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", DisplayName = "攻击数据表")
	TObjectPtr<UDataTable> AttackDataTable = nullptr;
	// 当前攻击伤害倍率
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Score", DisplayName = "当前攻击伤害倍率")
	float CurrentDamageModifier = 1.0f;
	// 下次攻击伤害倍率
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Score", DisplayName = "下次攻击伤害倍率")
	float NextAttackDamageModifier = 1.0f;

	// 切换攻击方向的最小间隔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Input", DisplayName = "切换攻击方向最小间隔")
	float DirectionSwitchCooldown = 0.25f;

	// 任意两次攻击请求的最小间隔，防止极短时间重复触发
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Input", DisplayName = "攻击请求最小间隔")
	float AttackRequestCooldown = 0.12f;

	// 连击窗口持续时间
	UFUNCTION(BlueprintPure, Category = "Combat")
	EAttackState GetAttackState() const { return AttackState; }

	// 当前攻击方向
	UFUNCTION(BlueprintPure, Category = "Combat")
	EAttackDirection GetCurrentDirection() const { return CurrentDirection; }

	// 攻击触发计数器
	UFUNCTION(BlueprintPure, Category = "Combat")
	int32 GetAttackTriggerCounter() const { return AttackTriggerCounter; }

	// 当前是否存在正在播放的攻击
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttackActive() const;

	// 当前攻击伤害倍率
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetCurrentDamageModifier() const { return CurrentDamageModifier; }

	// 当前攻击最终伤害
	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	float GetCurrentAttackDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void EnableWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void DisableWeaponTrace();

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	float GetCurrentBaseDamage() const { return CurrentBaseDamage; }

	UFUNCTION(BlueprintPure, Category = "Combat|Damage")
	FName GetCurrentAttackType() const { return CurrentAttackType; }

	bool bWeaponTraceWindowOpen = false;
	FName CurrentAttackType = NAME_None;

	// 是否按住攻击键
	bool bIsAttackKeyDown = false;
private:
	// 执行攻击
	void PerformAttack(EAttackDirection Direction, float TrackScore);
	// 根据方向查攻击表
	const FAttack* FindAttackRowByDirection(EAttackDirection InDirection) const;
	// 同步状态机
	void RefreshAttackState(float CurrentTime);
	// 当前是否存在正在播放的攻击
	bool HasActiveAttack(float CurrentTime) const;
	// 是否在采样状态
	bool IsSamplingState() const;
	// 是否在锁定状态
	bool IsLockedState() const;
	// 清理采样缓存
	void ClearSamplingBuffer();
	// 清理待定攻击
	void ClearPendingAttack();
	// 重新缓存动画实例
	bool CacheAnimInstance();
private:
	// 当前连击窗口时间
	float CurrentWindowTime = 0.0f;
	// 是否按下右键
	bool bIsBlocking = false;
	// 攻击开始时间
	float CurrentAttackStartTime = -1.f;
	// 攻击结束时间
	float CurrentAttackEndTime = -1.f;
	// 当前攻击方向
	EAttackDirection CurrentDirection = EAttackDirection::None;
	// 当前攻击状态
	EAttackState AttackState = EAttackState::Idle;
	// 是否有待定攻击
	bool bHasPendingAttack = false;
	// 待定攻击方向
	EAttackDirection PendingDirection = EAttackDirection::None;
	// 待定攻击评分
	float PendingTrackScore = 0.0f;

	FAttackValid AttackValid;
	// 当前动画实例
	TObjectPtr<UAnimInstance> Anim = nullptr;

	// 上一次被接受的攻击输入方向
	EAttackDirection LastAcceptedInputDirection = EAttackDirection::None;

	// 上一次被接受攻击输入的时间
	float LastAcceptedInputTime = -10000.0f;

	// 判断当前方向输入是否允许触发攻击请求
	bool CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const;

	// 记录一次已接受的攻击输入
	void MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime);

	// 当前攻击基础伤害，来自 DataTable 的 AttackRow->Damage
	float CurrentBaseDamage = 0.0f;

	// 攻击触发计数器，每次真正出招时 +1，AnimBP 用它判断是否触发新攻击
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	int32 AttackTriggerCounter = 0;

};


