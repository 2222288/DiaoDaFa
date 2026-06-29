#pragma once

#include "CoreMinimal.h"
#include "Weapon/WeaponTypes.h"

/** 战斗方向工具：统一八方向索引、环形距离、方向关系和中文文本。 */
class KERTYER_API FCombatDirectionUtils
{
public:
	static int32 DirectionToIndex(EAttackDirection Direction);
	static int32 CircularDirectionDelta(EAttackDirection A, EAttackDirection B);
	static EWeaponContactDirectionRelation ResolveDirectionRelation(EAttackDirection GuardDirection, EAttackDirection IncomingDirection);
	static const TCHAR* DirectionToChinese(EAttackDirection Direction);
	static const TCHAR* DirectionRelationToChinese(EWeaponContactDirectionRelation Relation);
};