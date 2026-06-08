#include "AnimNotify/Block/AnimNotify_BlockStart.h"
#include "Components/AttackComponent.h"
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

	if (UAttackComponent* AttackComponent = Owner->FindComponentByClass<UAttackComponent>())
	{
		AttackComponent->StartBlock();
	}
}