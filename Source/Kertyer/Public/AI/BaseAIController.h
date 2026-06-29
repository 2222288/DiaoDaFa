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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	void UpdateAI();

	AMy* GetPlayerCharacter();
	void EnterIdleState();
	void EnterAttackState(AMy* Player);
	void UpdateChaseState(AMy* Player);
	void RequestMoveToPlayer(AMy* Player, bool bForceRequest);

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

	//目标移动超过该距离后，才允许重新提交寻路。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Navigation")
	float RepathDistanceThreshold = 150.0f;

	//即使目标快速移动，两次主动重寻路之间也至少间隔这么久。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Navigation")
	float MinRepathInterval = 0.5f;

private:
	enum class ESimpleAIState : uint8
	{
		Idle,
		Chase,
		Attack
	};

	TWeakObjectPtr<AMy> CachedPlayerCharacter;
	ESimpleAIState CurrentState = ESimpleAIState::Idle;
	FVector LastMoveRequestLocation = FVector::ZeroVector;
	float LastMoveRequestTime = -10000.0f;
};