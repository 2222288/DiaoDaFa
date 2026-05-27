#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AttackTraceEnd.generated.h"

UCLASS(meta = (DisplayName = "Attack Trace End"))
class KERTYER_API UAnimNotify_AttackTraceEnd : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
    virtual FString GetNotifyName_Implementation() const override { return TEXT("AttackTraceEnd"); }
};