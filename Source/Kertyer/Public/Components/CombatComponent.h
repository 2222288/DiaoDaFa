#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "Combat/CombatTypes.h"

#include "Attackif/Attackif.h"

#include "Attackif/AttackValid.h"
#include "Combat/CombatStatusSwitch.h"
#include "Combat/CombatDamage.h"
#include "Combat/CombatSampling.h"
#include "CombatComponent.generated.h"

class UAttackMoveDataAsset;
struct FAttackMoveData;


/**
 * 攻击组件：
 * 负责接收鼠标轨迹输入、判断攻击方向、维护攻击状态、
 * 并通过角色把本次攻击数据下发给武器。
 *
 * 动画播放逻辑已经拆分到 AnimationLogic/AttackAnimationPlayer。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 构造函数：开启组件 Tick。 */
	UCombatComponent();

	/** 游戏开始时初始化攻击状态和缓存。 */
	virtual void BeginPlay() override;

	/** 每帧刷新攻击状态，处理待定攻击，并在攻击结束时关闭武器判定窗口。 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	///** 缓存鼠标输入，并交给 AttackValid 判断是否形成一次有效攻击。 */
	//void CacheMouseInput(const FVector2D& Input, float CurrentTime);

	///** 按下攻击键时开始采样攻击轨迹。 */
	//void BeginAttackSampling(float CurrentTime);

	///** 松开攻击键时结束采样并清理输入缓存。 */
	//void EndAttackSampling();

	///** 开始格挡。通常由格挡动画通知调用。 */
	//void StartBlock();

	///** 停止格挡。通常由格挡动画通知调用。 */
	//void StopBlock();

	///** 当前是否处于格挡有效段。 */
	//bool IsBlocking() const { return CombatStatusSwitch.bIsBlocking; }


	////////////////////////////////
	FCombatStatusSwitch CombatStatusSwitch;

	FCombatSampling CombatSampling;

	FCombatDamage CombatDamage;

public:
	///** 鼠标移动采样的最小阈值，低于该距离的输入会被过滤。 */
	//float MinSampleDistance = 8.0f;

	/** 攻击动作数据资产，用方向查找对应攻击动画、伤害和窗口期。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "攻击动作数据资产"))
	TObjectPtr<UAttackMoveDataAsset> AttackMoveDataAsset = nullptr;

	///** 当前这一次攻击实际使用的伤害倍率。 */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Score", meta = (DisplayName = "当前攻击伤害倍率"))
	//float CurrentDamageModifier = 1.0f;

	///** 下一次攻击将会使用的伤害倍率，由本次轨迹评分写入。 */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Score", meta = (DisplayName = "下次攻击伤害倍率"))
	//float NextAttackDamageModifier = 1.0f;

	///** 切换攻击方向的最小间隔，防止极短时间内连续变向触发。 */
	//float DirectionSwitchCooldown = 0.25f;
	///** 任意两次攻击请求的最小间隔，防止重复触发。 */

	//float AttackRequestCooldown = 0.12f;

	///** 从攻击输入被接受到真正出招的最大允许时间，超过则攻击请求失效。 */
	//float CounterAttackValidWindow = 0.5f;


	///** 获取当前攻击状态。 */
	//UFUNCTION(BlueprintPure, Category = "Combat", meta = (DisplayName = "获取攻击状态"))
	//EAttackState GetAttackState() const { return CombatStatusSwitch.AttackState; }

	///** 获取当前攻击方向。 */
	//UFUNCTION(BlueprintPure, Category = "Combat", meta = (DisplayName = "获取当前攻击方向"))
	//EAttackDirection GetCurrentDirection() const { return CombatStatusSwitch.CurrentDirection; }

	//int32 GetAttackTriggerCounter() const { return AttackTriggerCounter; }

	///** 当前是否存在正在播放或尚未结束的攻击。 */
	//bool IsAttackActive() const;

	///** 获取当前攻击伤害倍率。 */
	//UFUNCTION(BlueprintPure, Category = "Combat", meta = (DisplayName = "获取当前攻击伤害倍率"))
	//float GetCurrentDamageModifier() const { return CurrentDamageModifier; }

	///** 获取当前攻击最终伤害，等于基础伤害乘以当前倍率。 */
	//UFUNCTION(BlueprintPure, Category = "Combat|Damage", meta = (DisplayName = "获取当前攻击最终伤害"))
	//float GetCurrentAttackDamage() const;

	///** 打开当前武器的命中判定窗口。通常由动画通知调用。 */
	//UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "打开武器判定窗口"))
	//void EnableWeaponTrace();

	///** 关闭当前武器的命中判定窗口。通常由动画通知调用。 */
	//UFUNCTION(BlueprintCallable, Category = "Combat|Weapon", meta = (DisplayName = "关闭武器判定窗口"))
	//void DisableWeaponTrace();

	////中断当前攻击
	//UFUNCTION(BlueprintCallable, Category = "Combat|Interrupt", meta = (DisplayName = "打断当前攻击"))
	//void InterruptCurrentAttack();

	///** 获取当前攻击基础伤害，来自攻击数据表。 */
	//UFUNCTION(BlueprintPure, Category = "Combat|Damage", meta = (DisplayName = "获取当前攻击基础伤害"))
	//float GetCurrentBaseDamage() const { return CombatDamage.CurrentBaseDamage; }

	////** 获取当前攻击 ID。 */
	//FName GetCurrentAttackType() const { return CurrentAttackType; }

	////开始弹刀
	//UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "开始弹刀"))
	//void StartDeflect();

	////结束弹刀
	//UFUNCTION(BlueprintCallable, Category = "Combat|Deflect", meta = (DisplayName = "结束弹刀"))
	//void EndDeflect();

	////是否在弹刀中
	//UFUNCTION(BlueprintPure, Category = "Combat|Deflect", meta = (DisplayName = "当前是否正在弹刀"))
	//bool IsDeflecting() const { return bIsDeflecting; }

	//// 本次攻击被攻击前判定转换为格挡后，记录组件状态。
	//void StartConvertedGuard(EAttackDirection Direction, float CurrentTime, float GuardDuration);

	///** 当前武器判定窗口是否已经打开。 */
	//bool bWeaponTraceWindowOpen = false;

	///** 当前攻击 ID */
	//FName CurrentAttackType = NAME_None;

	///** 当前是否按住攻击键。 */
	//bool bIsAttackKeyDown = false;

private:

	///** 执行一次攻击：请求动画播放、更新伤害数据、下发武器攻击数据。 */
	//void PerformAttack(EAttackDirection Direction, float TrackScore);

	///** 根据攻击方向从攻击动作数据资产查找攻击动作。 */
	//const FAttackMoveData* FindAttackMoveByDirection(EAttackDirection InDirection) const;

	////是否在弹刀中
	//bool bIsDeflecting = false;

	/////////////////////////////////////
	///** 按当前时间刷新攻击状态机。 */
	//void RefreshAttackState(float CurrentTime);
	//////////////////////////////////////
	///** 判断当前是否有未结束的攻击。 */
	//bool HasActiveAttack(float CurrentTime) const;


	///** 判断当前状态是否允许采样输入。 */
	//bool IsSamplingState() const;

	///** 判断当前状态是否处于攻击锁定期。 */
	//bool IsLockedState() const;

	///** 清理当前输入轨迹缓存。 */
	//void ClearSamplingBuffer();

	///** 清理待定攻击数据。 */
	//void ClearPendingAttack();

	///** 判断当前方向输入是否允许触发攻击请求。 */
	//bool CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const;

	///** 记录一次已经被接受的攻击输入。 */
	//void MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime);

private:
	///** 当前连击窗口时间。 */
	//float CurrentWindowTime = 0.0f;

	///** 当前是否正在格挡。 */
	//bool bIsBlocking = false;

	///** 当前攻击开始时间。 */
	//float CurrentAttackStartTime = -1.f;

	///** 当前攻击结束时间。 */
	//float CurrentAttackEndTime = -1.f;

	///** 当前攻击方向。 */
	//EAttackDirection CurrentDirection = EAttackDirection::None;

	///** 当前攻击状态。 */
	//EAttackState AttackState = EAttackState::Idle;

	///** 是否存在待定攻击。 */
	//bool bHasPendingAttack = false;

	///** 待定攻击方向。 */
	//EAttackDirection PendingDirection = EAttackDirection::None;

	///** 待定攻击轨迹评分。 */
	//float PendingTrackScore = 0.0f;

	///** 攻击输入有效性判断器。 */
	//FAttackValid AttackValid;

	///** 上一次被接受的攻击输入方向。 */
	//EAttackDirection LastAcceptedInputDirection = EAttackDirection::None;

	///** 上一次被接受攻击输入的时间。 */
	//float LastAcceptedInputTime = -10000.0f;

	///** 当前攻击基础伤害 */
	//float CurrentBaseDamage = 0.0f;

	///** 攻击触发计数器，每次真正出招时 +1，AnimBP 用它判断是否触发新攻击。 */
	//int32 AttackTriggerCounter = 0;
};