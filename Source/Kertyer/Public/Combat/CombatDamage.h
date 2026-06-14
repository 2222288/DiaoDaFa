#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Attackif/Attackif.h"
#include "Attackif/AttackValid.h"
#include "Math/Vector2D.h"

class FCombatDamage
{
public:
	/** 当前这一次攻击实际使用的伤害倍率。 */
	float CurrentDamageModifier = 1.0f;

	/** 下一次攻击将会使用的伤害倍率，由本次轨迹评分写入。 */
	float NextAttackDamageModifier = 1.0f;

	/** 获取当前攻击伤害倍率。 */
	float GetCurrentDamageModifier() const { return CurrentDamageModifier; }

	/** 获取当前攻击最终伤害，等于基础伤害乘以当前倍率。 */
	float GetCurrentAttackDamage() const;

	/** 打开当前武器的命中判定窗口。通常由动画通知调用。 */
	void EnableWeaponTrace();

	/** 关闭当前武器的命中判定窗口。通常由动画通知调用。 */
	void DisableWeaponTrace();

	/** 当前武器判定窗口是否已经打开。 */
	bool bWeaponTraceWindowOpen = false;

	/** 当前攻击 ID */
	FName CurrentAttackType = NAME_None;

	/** 执行一次攻击：请求动画播放、更新伤害数据、下发武器攻击数据。 */
	void PerformAttack(EAttackDirection Direction, float TrackScore);

	/** 当前攻击基础伤害 */
	float CurrentBaseDamage = 0.0f;

	/** 获取当前攻击基础伤害，来自攻击数据表。 */
	float GetCurrentBaseDamage() const { return CurrentBaseDamage; }

};