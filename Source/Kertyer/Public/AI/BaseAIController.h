#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BaseAIController.generated.h"

class AHostile;
class AMy;

UCLASS()
class KERTYER_API ABaseAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
	void UpdateAI();

	AMy* GetPlayerCharacter() const;

	void FaceTarget(AActor* TargetActor);

protected:
	FTimerHandle AIUpdateTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectRange = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MoveAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float UpdateInterval = 0.15f;
};