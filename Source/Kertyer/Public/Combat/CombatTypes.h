#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

/** 八方向攻击方向。 */
UENUM(BlueprintType)
enum class EAttackDirection : uint8
{
	None UMETA(DisplayName = "无"),
	Up UMETA(DisplayName = "上"),
	UpRight UMETA(DisplayName = "右上"),
	Right UMETA(DisplayName = "右"),
	DownRight UMETA(DisplayName = "右下"),
	Down UMETA(DisplayName = "下"),
	DownLeft UMETA(DisplayName = "左下"),
	Left UMETA(DisplayName = "左"),
	UpLeft UMETA(DisplayName = "左上")
};

/** 攻击状态机状态。 */
UENUM(BlueprintType)
enum class EAttackState : uint8
{
	Idle UMETA(DisplayName = "空闲"),
	Sampling UMETA(DisplayName = "采样中"),
	AttackingLocked UMETA(DisplayName = "攻击锁定"),
	ComboWindowOpen UMETA(DisplayName = "连击窗口打开"),
	SamplingLocked UMETA(DisplayName = "采样但锁定"),
	SamplingComboWindow UMETA(DisplayName = "采样且连击窗口打开")
};

/** 战斗反应类型。 */
UENUM(BlueprintType)
enum class ECombatReactionType : uint8
{
	None UMETA(DisplayName = "None"),
	Guard UMETA(DisplayName = "格挡"),
	Hit UMETA(DisplayName = "受击"),
	Clash UMETA(DisplayName = "武器碰撞"),
	Deflect UMETA(DisplayName = "弹刀"),
	Interrupt UMETA(DisplayName = "打断"),
	Death UMETA(DisplayName = "死亡")
};