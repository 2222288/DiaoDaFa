#pragma once



#include "CoreMinimal.h"

#include "Animation/AnimNotifies/AnimNotify.h"

#include "AnimNotify_AttackTraceStart.generated.h"



UCLASS(meta = (DisplayName = "DEPRECATED Attack Trace Start - Use Attack Damage Window"))

class KERTYER_API UAnimNotify_AttackTraceStart : public UAnimNotify

{

    GENERATED_BODY()



public:

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override { return TEXT("AttackTraceStart"); }

};