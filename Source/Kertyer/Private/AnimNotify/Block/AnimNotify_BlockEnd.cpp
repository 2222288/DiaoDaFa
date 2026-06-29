#include "AnimNotify/Block/AnimNotify_BlockEnd.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BlockEnd::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
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
		CombatComponent->StopBlock();
	}
}