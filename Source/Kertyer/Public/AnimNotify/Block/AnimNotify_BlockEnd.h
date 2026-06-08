#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BlockEnd.generated.h"

UCLASS(meta = (DisplayName = "Block End"))
class KERTYER_API UAnimNotify_BlockEnd : public UAnimNotify
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
		return TEXT("BlockEnd");
	}
};