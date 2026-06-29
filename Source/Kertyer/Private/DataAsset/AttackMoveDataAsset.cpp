#include "DataAsset/AttackMoveDataAsset.h"

const FAttackMoveData* UAttackMoveDataAsset::FindAttackByDirection(EAttackDirection Direction) const
{
	if (Direction == EAttackDirection::None)
	{
		return nullptr;
	}

	for (const FAttackMoveData& AttackMove : AttackMoves)
	{
		if (AttackMove.AttackDirection == Direction)
		{
			return &AttackMove;
		}
	}

	return nullptr;
}

const FAttackMoveData* UAttackMoveDataAsset::GetRandomAttack() const
{
	if (AttackMoves.Num() <= 0)
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, AttackMoves.Num() - 1);
	return &AttackMoves[RandomIndex];
}