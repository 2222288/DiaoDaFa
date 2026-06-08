#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatTypes.h"
#include "Weapon/WeaponTypes.h"
#include "CombatReactionAnimationData.generated.h"

/**
 * 战斗反应动画配置。
 *
 * 只描述结算之后的反应动画。
 * 不描述主动攻击动画。
 */
USTRUCT(BlueprintType)
struct KERTYER_API FCombatReactionAnimation
{
	GENERATED_BODY()

	/** 反应类型：Guard / Hit / Deflect / Interrupt / Death。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	ECombatReactionType ReactionType = ECombatReactionType::None;

	/** 武器接触结果。Ignore 表示不限定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	EWeaponContactResult ContactResult = EWeaponContactResult::Ignore;

	/** 方向。None 表示不限定方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	EAttackDirection Direction = EAttackDirection::None;

	/** 是否要求自己是较慢方。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	bool bRequireSelfSlower = false;

	/** 是否要求处于有效响应窗口内。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	bool bRequireValidTimedResponse = false;

	/** 反应动画 Montage。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** Montage Section。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reaction")
	FName MontageSection = NAME_None;

	/** 播放倍率。 */
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