#include "AnimNotify/Block/AnimNotify_BlockStart.h"
#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_BlockStart::Notify(
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
		CombatComponent->StartBlock();
	}
}