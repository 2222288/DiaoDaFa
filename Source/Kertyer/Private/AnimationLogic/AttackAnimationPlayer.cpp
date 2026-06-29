#include "AnimationLogic/AttackAnimationPlayer.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAsset/AttackMoveDataAsset.h"
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
    return Mesh ? Mesh->GetAnimInstance() : nullptr;
}

FAttackAnimationPlayResult FAttackAnimationPlayer::PlayAttackMontage(
    AActor* Owner,
    const FAttackMoveData& AttackData,
    float PlayRate
)
{
    return PlayRawMontage(
        Owner,
        AttackData.AttackMontage.Get(),
        AttackData.MontageSection,
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
    Result.PlayedLength = ActualPlayedLength; // 仅供日志、显示使用，不再决定攻击结束。
    Result.PlayedMontage = Montage;
    Result.PlayedSection = MontageSection;
    return Result;
}

bool FAttackAnimationPlayer::StopAttackMontage(
    AActor* Owner,
    UAnimMontage* Montage,
    float BlendOutTime
)
{
    if (!Montage)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[攻击动画][停止失败] 未指定攻击蒙太奇，已拒绝停止全部蒙太奇。Owner=%s"),
            *GetNameSafe(Owner)
        );
        return false;
    }

    UAnimInstance* AnimInstance = ResolveAnimInstance(Owner);
    if (!AnimInstance)
    {
        return false;
    }

    AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), Montage);
    return true;
}
