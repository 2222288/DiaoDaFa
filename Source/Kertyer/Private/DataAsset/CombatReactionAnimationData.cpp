#include "DataAsset/CombatReactionAnimationData.h"

const FCombatReactionAnimation* UCombatReactionAnimationDataAsset::FindBestReaction(
	ECombatReactionType ReactionType,
	EWeaponContactResult ContactResult,
	EAttackDirection Direction,
	bool bSelfIsSlower,
	bool bValidTimedResponse
) const
{
	const FCombatReactionAnimation* Fallback = nullptr;

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

		if (Row.Direction == Direction)
		{
			return &Row;
		}

		if (!Fallback)
		{
			Fallback = &Row;
		}
	}

	return Fallback;
}
