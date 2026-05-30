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

	//检测范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectRange = 1200.0f;

	//攻击范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 300.0f;

	//移动接受半径
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float MoveAcceptanceRadius = 120.0f;

	//AI更新间隔
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float UpdateInterval = 0.15f;
};