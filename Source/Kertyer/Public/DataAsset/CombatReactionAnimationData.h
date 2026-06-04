#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "DataAsset/AttackDH.h"
#include "Weapon/WeaponTypes.h"
#include "CombatReactionAnimationData.generated.h"

UENUM(BlueprintType)
enum class ECombatReactionType : uint8
{
	None UMETA(DisplayName = "None"),
	Guard UMETA(DisplayName = "Guard"),
	Hit UMETA(DisplayName = "Hit"),
	Clash UMETA(DisplayName = "Clash"),
	Deflect UMETA(DisplayName = "Deflect"),
	Interrupt UMETA(DisplayName = "Interrupt"),
	Death UMETA(DisplayName = "Death")
};

USTRUCT(BlueprintType)
struct KERTYER_API FCombatReactionAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	ECombatReactionType ReactionType = ECombatReactionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	EWeaponContactResult ContactResult = EWeaponContactResult::Ignore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	EAttackDirection Direction = EAttackDirection::None;

	// True means this row only matches when this character is the slower side.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	bool bRequireSelfSlower = false;

	// True means this row only matches inside the valid timed response window.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	bool bRequireValidTimedResponse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	FName MontageSection = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	float PlayRate = 1.0f;
};

UCLASS(BlueprintType)
class KERTYER_API UCombatReactionAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	TArray<FCombatReactionAnimation> ReactionAnimations;

	const FCombatReactionAnimation* FindBestReaction(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult,
		EAttackDirection Direction,
		bool bSelfIsSlower,
		bool bValidTimedResponse
	) const;
};
