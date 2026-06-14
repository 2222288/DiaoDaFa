#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Attackif/AttackValid.h" 
#include "Math/UnrealMathUtility.h"
#include "Combat/CombatSampling.h"
#include "Math/Vector2D.h"


class FCombatStatusSwitch
{
public:

	AActor* CSActor;

	float CSDeltaTime;

	UWorld* CSWorld;

	void GetActor(float DeltaTime, AActor* Actor, UWorld* World);

	//按当前时间刷新攻击状态机
	void RefreshAttackState(float CurrentTime);

	//判断当前是否有未结束的攻击。 
	bool HasActiveAttack(float CurrentTime) const;

	// 本次攻击被攻击前判定转换为格挡后，记录组件状态。
	void StartConvertedGuard(EAttackDirection Direction, float CurrentTime, float GuardDuration);
	/** 当前武器判定窗口是否已经打开。 */
	bool bWeaponTraceWindowOpen = false;

	/** 当前攻击 ID。 */
	FName CurrentAttackType = NAME_None;

	/** 当前是否按住攻击键。 */
	bool bIsAttackKeyDown = false;

	//中断当前攻击
	UFUNCTION(BlueprintCallable, Category = "Combat|Interrupt", meta = (DisplayName = "打断当前攻击"))
	void InterruptCurrentAttack();

	/** 获取当前攻击 ID。 */
	FName GetCurrentAttackType() const { return CurrentAttackType; }


	/** 判断当前状态是否允许采样输入。 */
	bool IsSamplingState() const;

	/** 判断当前状态是否处于攻击锁定期。 */
	bool IsLockedState() const;

	/** 判断当前方向输入是否允许触发攻击请求。 */
	bool CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const;

	/** 当前连击窗口时间。 */
	float CurrentWindowTime = 0.0f;

	/** 当前是否正在格挡。 */
	bool bIsBlocking = false;

	/** 当前攻击开始时间。 */
	float CurrentAttackStartTime = -1.f;

	/** 当前攻击结束时间。 */
	float CurrentAttackEndTime = -1.f;

	/** 当前攻击方向。 */
	EAttackDirection CurrentDirection = EAttackDirection::None;

	/** 当前攻击状态。 */
	EAttackState AttackState = EAttackState::Idle;

	/** 是否存在待定攻击。 */
	bool bHasPendingAttack = false;

	/** 待定攻击方向。 */
	EAttackDirection PendingDirection = EAttackDirection::None;

	/** 攻击输入有效性判断器。 */
	FAttackValid AttackValid;

	/** 上一次被接受的攻击输入方向。 */
	EAttackDirection LastAcceptedInputDirection = EAttackDirection::None;

	/** 上一次被接受攻击输入的时间。 */
	float LastAcceptedInputTime = -10000.0f;


	//获取攻击触发计数器的值
	int32 GetAttackTriggerCounter() const { return AttackTriggerCounter; }

	/** 攻击触发计数器，每次真正出招时 +1，AnimBP 用它判断是否触发新攻击。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true", DisplayName = "攻击触发计数器"))
	int32 AttackTriggerCounter = 0;

	/** 记录一次已经被接受的攻击输入。 */
	void MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime);

	/** 开始格挡。通常由格挡动画通知调用。 */
	void StartBlock();

	/** 停止格挡。通常由格挡动画通知调用。 */
	void StopBlock();

	/** 当前是否处于格挡有效段。 */
	bool IsBlocking() const { return bIsBlocking; }

	/** 切换攻击方向的最小间隔，防止极短时间内连续变向触发。 */
	float DirectionSwitchCooldown = 0.25f;
	/** 任意两次攻击请求的最小间隔，防止重复触发。 */

	float AttackRequestCooldown = 0.12f;

	/** 从攻击输入被接受到真正出招的最大允许时间，超过则攻击请求失效。 */
	float CounterAttackValidWindow = 0.5f;

	/** 获取当前攻击状态。 */
	EAttackState GetAttackState() const { return AttackState; }

	/** 获取当前攻击方向。 */
	EAttackDirection GetCurrentDirection() const { return CurrentDirection; }

	/** 当前是否存在正在播放或尚未结束的攻击。 */
	bool IsAttackActive() const;

	// 本次攻击被攻击前判定转换为格挡后，记录组件状态。
	void StartConvertedGuard(EAttackDirection Direction, float CurrentTime, float GuardDuration);

	//开始弹刀
	void StartDeflect();

	//结束弹刀
	void EndDeflect();

	//是否在弹刀中
	bool IsDeflecting() const { return bIsDeflecting; }

	//是否在弹刀中
	bool bIsDeflecting = false;

	/** 根据攻击方向从攻击动作数据资产查找攻击动作。 */
	const FAttackMoveData* FindAttackMoveByDirection(EAttackDirection InDirection) const;
};