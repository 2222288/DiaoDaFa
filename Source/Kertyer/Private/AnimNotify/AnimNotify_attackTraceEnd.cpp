#include "AnimNotify/AnimNotify_AttackTraceEnd.h"
#include "Character/Base.h"
#include "Components/AttackComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackTraceEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

    if (UAttackComponent* AttackComponent = Owner->FindComponentByClass<UAttackComponent>())
    {
        AttackComponent->DisableWeaponTrace();
        return;
    }

    if (ABase* BaseCharacter = Cast<ABase>(Owner))
    {
        BaseCharacter->DisableWeaponTrace();
    }
}