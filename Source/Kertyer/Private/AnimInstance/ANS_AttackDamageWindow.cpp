#include "AnimInstance/ANS_AttackDamageWindow.h"

#include "Character/Base.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
    void SetAttackTraceEnabled(USkeletalMeshComponent* MeshComp, bool bEnabled)
    {
        if (!MeshComp)
        {
            return;
        }

        AActor* Owner = MeshComp->GetOwner();
        if (!Owner)
        {
            return;
        }

        if (UCombatComponent* AttackComp = Owner->FindComponentByClass<UCombatComponent>())
        {
            bEnabled ? AttackComp->EnableWeaponTrace() : AttackComp->DisableWeaponTrace();
            return;
        }

        // AHostile 没有 UCombatComponent，走 ABase 的武器接口。
        if (ABase* BaseCharacter = Cast<ABase>(Owner))
        {
            bEnabled ? BaseCharacter->EnableWeaponTrace() : BaseCharacter->DisableWeaponTrace();
        }
    }
}

void UANS_AttackDamageWindow::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    SetAttackTraceEnabled(MeshComp, true);
    UE_LOG(LogTemp, Verbose, TEXT("攻击有效帧开始：开启武器轨迹"));
}

void UANS_AttackDamageWindow::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    SetAttackTraceEnabled(MeshComp, false);
    UE_LOG(LogTemp, Verbose, TEXT("攻击有效帧结束：关闭武器轨迹"));
}