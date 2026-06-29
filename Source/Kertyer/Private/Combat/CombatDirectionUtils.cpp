#include "Combat/CombatDirectionUtils.h"

int32 FCombatDirectionUtils::DirectionToIndex(EAttackDirection Direction)
{
	switch (Direction)
	{
	case EAttackDirection::Up: return 0;
	case EAttackDirection::UpRight: return 1;
	case EAttackDirection::Right: return 2;
	case EAttackDirection::DownRight: return 3;
	case EAttackDirection::Down: return 4;
	case EAttackDirection::DownLeft: return 5;
	case EAttackDirection::Left: return 6;
	case EAttackDirection::UpLeft: return 7;
	default: return INDEX_NONE;
	}
}

int32 FCombatDirectionUtils::CircularDirectionDelta(EAttackDirection A, EAttackDirection B)
{
	const int32 IndexA = DirectionToIndex(A);
	const int32 IndexB = DirectionToIndex(B);

	if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 RawDelta = FMath::Abs(IndexA - IndexB);
	return FMath::Min(RawDelta, 8 - RawDelta);
}

EWeaponContactDirectionRelation FCombatDirectionUtils::ResolveDirectionRelation(EAttackDirection GuardDirection, EAttackDirection IncomingDirection)
{
	const int32 Delta = CircularDirectionDelta(GuardDirection, IncomingDirection);

	if (Delta == 4)
	{
		return EWeaponContactDirectionRelation::Opposite;
	}

	if (Delta == 3)
	{
		return EWeaponContactDirectionRelation::NearOpposite;
	}

	if (Delta != INDEX_NONE)
	{
		return EWeaponContactDirectionRelation::NonOpposite;
	}

	return EWeaponContactDirectionRelation::Invalid;
}

const TCHAR* FCombatDirectionUtils::DirectionToChinese(EAttackDirection Direction)
{
	switch (Direction)
	{
	case EAttackDirection::None: return TEXT("无");
	case EAttackDirection::Up: return TEXT("上");
	case EAttackDirection::UpRight: return TEXT("右上");
	case EAttackDirection::Right: return TEXT("右");
	case EAttackDirection::DownRight: return TEXT("右下");
	case EAttackDirection::Down: return TEXT("下");
	case EAttackDirection::DownLeft: return TEXT("左下");
	case EAttackDirection::Left: return TEXT("左");
	case EAttackDirection::UpLeft: return TEXT("左上");
	default: return TEXT("未知方向");
	}
}

const TCHAR* FCombatDirectionUtils::DirectionRelationToChinese(EWeaponContactDirectionRelation Relation)
{
	switch (Relation)
	{
	case EWeaponContactDirectionRelation::Opposite: return TEXT("对向格挡");
	case EWeaponContactDirectionRelation::NearOpposite: return TEXT("偏对向格挡");
	case EWeaponContactDirectionRelation::NonOpposite: return TEXT("错误方向格挡");
	case EWeaponContactDirectionRelation::Invalid: return TEXT("无效方向格挡");
	default: return TEXT("未知格挡");
	}
}