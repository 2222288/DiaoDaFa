#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatTypes.h"
#include "Weapon/WeaponTypes.h"
#include "CombatReactionComponent.generated.h"

class UCombatReactionAnimationDataAsset;

/** 战斗反应组件：负责 Hit、Guard、Deflect、Interrupt、Death 等反应动画播放。 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UCombatReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatReactionComponent();

	//战斗反应动画数据
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction")
	TObjectPtr<UCombatReactionAnimationDataAsset> CombatReactionAnimationData = nullptr;

	//游戏战斗反应
	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	bool PlayCombatReaction(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult = EWeaponContactResult::Ignore,
		EAttackDirection Direction = EAttackDirection::None,
		bool bSelfIsSlower = false,
		bool bValidTimedResponse = false
	);

	//游戏战斗反应并获取动画长度
	bool PlayCombatReactionAndGetLength(
		ECombatReactionType ReactionType,
		EWeaponContactResult ContactResult,
		EAttackDirection Direction,
		bool bSelfIsSlower,
		bool bValidTimedResponse,
		float& OutPlayedLength
	);

	//游戏武器接触反应
	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayWeaponContactReaction(
		const FWeaponContactResolveOutput& ResolveOutput,
		EWeaponContactSide SelfSide
	);

	//游戏受击反应
	UFUNCTION(BlueprintCallable, Category = "Combat|Reaction")
	void PlayHitReaction();
};