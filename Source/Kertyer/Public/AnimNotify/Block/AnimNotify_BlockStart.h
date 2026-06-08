#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BlockStart.generated.h"

UCLASS(meta = (DisplayName = "Block Start"))
class KERTYER_API UAnimNotify_BlockStart : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("BlockStart");
	}
};