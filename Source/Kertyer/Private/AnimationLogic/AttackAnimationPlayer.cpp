#include "AnimationLogic/AttackAnimationPlayer.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAsset/AttackDH.h"
#include "DataAsset/CombatReactionAnimationData.h"
#include "GameFramework/Character.h"

UAnimInstance* FAttackAnimationPlayer::ResolveAnimInstance(AActor* Owner)
{
	if (!Owner)
	{
		return nullptr;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return nullptr;
	}

	return Mesh->GetAnimInstance();
}

FAttackAnimationPlayResult FAttackAnimationPlayer::PlayAttackMontage(
	AActor* Owner,
	const FAttack& AttackRow,
	float PlayRate
)
{
	return PlayRawMontage(
		Owner,
		AttackRow.AttackMontage.Get(),
		AttackRow.MontageSection,
		PlayRate
	);
}

FAttackAnimationPlayResult FAttackAnimationPlayer::PlayReactionMontage(
	AActor* Owner,
	const FCombatReactionAnimation& ReactionRow
)
{
	return PlayRawMontage(
		Owner,
		ReactionRow.Montage.Get(),
		ReactionRow.MontageSection,
		ReactionRow.PlayRate
	);
}

FAttackAnimationPlayResult FAttackAnimationPlayer::PlayRawMontage(
	AActor* Owner,
	UAnimMontage* Montage,
	FName MontageSection,
	float PlayRate
)
{
	FAttackAnimationPlayResult Result;

	if (PlayRate <= 0.0f)
	{
		Result.ErrorMessage = TEXT("PlayRate must be greater than 0.");
		return Result;
	}

	if (!Montage)
	{
		Result.ErrorMessage = TEXT("Montage is null.");
		return Result;
	}

	UAnimInstance* AnimInstance = ResolveAnimInstance(Owner);
	if (!AnimInstance)
	{
		Result.ErrorMessage = TEXT("AnimInstance is invalid.");
		return Result;
	}

	int32 SectionIndex = INDEX_NONE;

	if (MontageSection != NAME_None)
	{
		SectionIndex = Montage->GetSectionIndex(MontageSection);

		if (SectionIndex == INDEX_NONE)
		{
			Result.ErrorMessage = FString::Printf(
				TEXT("Montage section does not exist: %s"),
				*MontageSection.ToString()
			);
			return Result;
		}
	}

	const float MontagePlayedLength = AnimInstance->Montage_Play(Montage, PlayRate);
	if (MontagePlayedLength <= 0.0f)
	{
		Result.ErrorMessage = TEXT("Montage play failed.");
		return Result;
	}

	float ActualPlayedLength = MontagePlayedLength;

	if (SectionIndex != INDEX_NONE)
	{
		AnimInstance->Montage_JumpToSection(MontageSection, Montage);
		ActualPlayedLength = Montage->GetSectionLength(SectionIndex) / PlayRate;
	}

	Result.bSucceeded = true;
	Result.PlayedLength = ActualPlayedLength;
	Result.PlayedMontage = Montage;
	Result.PlayedSection = MontageSection;
	Result.ErrorMessage.Reset();

	return Result;
}

void FAttackAnimationPlayer::StopAttackMontage(AActor* Owner, float BlendOutTime)
{
	StopAttackMontage(Owner, nullptr, BlendOutTime);
}

void FAttackAnimationPlayer::StopAttackMontage(
	AActor* Owner,
	UAnimMontage* Montage,
	float BlendOutTime
)
{
	UAnimInstance* AnimInstance = ResolveAnimInstance(Owner);
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Stop(BlendOutTime, Montage);
}
