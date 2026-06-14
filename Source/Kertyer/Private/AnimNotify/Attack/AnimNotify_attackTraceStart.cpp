#include "AnimNotify/Attack/AnimNotify_AttackTraceStart.h"
#include "Character/Base.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackTraceStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    if (UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>())
    {
        CombatComponent->EnableWeaponTrace();
        return;
    }

    if (ABase* BaseCharacter = Cast<ABase>(Owner))
    {
        BaseCharacter->EnableWeaponTrace();
    }
}