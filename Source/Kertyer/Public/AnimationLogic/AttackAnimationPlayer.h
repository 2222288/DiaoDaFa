#pragma once

#include "CoreMinimal.h"

class UAnimInstance;
class UAnimMontage;
class AActor;

struct FAttackMoveData;
struct FCombatReactionAnimation;

struct FAttackAnimationPlayResult
{
	bool bSucceeded = false;
	float PlayedLength = 0.0f;
	UAnimMontage* PlayedMontage = nullptr;
	FName PlayedSection = NAME_None;
	FString ErrorMessage;
};

class KERTYER_API FAttackAnimationPlayer
{
public:
	static UAnimInstance* ResolveAnimInstance(AActor* Owner);

	static FAttackAnimationPlayResult PlayAttackMontage(
		AActor* Owner,
		const FAttackMoveData& AttackData,
		float PlayRate = 1.0f
	);

	static FAttackAnimationPlayResult PlayReactionMontage(
		AActor* Owner,
		const FCombatReactionAnimation& ReactionRow
	);

	static FAttackAnimationPlayResult PlayRawMontage(
		AActor* Owner,
		UAnimMontage* Montage,
		FName MontageSection = NAME_None,
		float PlayRate = 1.0f
	);

	static void StopAttackMontage(
		AActor* Owner,
		float BlendOutTime = 0.10f
	);

	static void StopAttackMontage(
		AActor* Owner,
		UAnimMontage* Montage,
		float BlendOutTime = 0.10f
	);
};
