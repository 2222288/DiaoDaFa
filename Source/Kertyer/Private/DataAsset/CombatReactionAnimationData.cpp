#include "DataAsset/CombatReactionAnimationData.h"

const FCombatReactionAnimation* UCombatReactionAnimationDataAsset::FindBestReaction(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse
) const
{
	const FCombatReactionAnimation* BestRow = nullptr;
	int32 BestScore = MIN_int32;

	for (const FCombatReactionAnimation& Row : ReactionAnimations)
	{
		if (Row.ReactionType != ReactionType)
		{
			continue;
		}

		if (!Row.Montage)
		{
			continue;
		}

		if (Row.ContactResult != EWeaponContactResult::Ignore && Row.ContactResult != ContactResult)
		{
			continue;
		}

		if (Row.Direction != EAttackDirection::None && Row.Direction != Direction)
		{
			continue;
		}

		if (Row.bRequireSelfSlower && !bSelfIsSlower)
		{
			continue;
		}

		if (Row.bRequireValidTimedResponse && !bValidTimedResponse)
		{
			continue;
		}

		int32 Score = 0;

		if (Row.ContactResult == ContactResult)
		{
			Score += 30;
		}

		if (Row.Direction == Direction)
		{
			Score += 30;
		}

		if (Row.bRequireSelfSlower == bSelfIsSlower)
		{
			Score += 10;
		}

		if (Row.bRequireValidTimedResponse == bValidTimedResponse)
		{
			Score += 10;
		}

		if (!BestRow || Score > BestScore)
		{
			BestRow = &Row;
			BestScore = Score;
		}
	}

	return BestRow;
}