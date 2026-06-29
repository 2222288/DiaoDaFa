#include "AI/BaseAIController.h"

#include "Character/Hostile.h"
#include "Character/My.h"

#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"

void ABaseAIController::BeginPlay()
{
	Super::BeginPlay();

	const float SafeUpdateInterval = FMath::Max(0.05f, UpdateInterval);
	const float FirstDelay = FMath::FRandRange(0.0f, SafeUpdateInterval);

	// 随机错开首次更新时间，避免大量 AI 在同一帧同时寻路。
	GetWorldTimerManager().SetTimer(
		AIUpdateTimer,
		this,
		&ABaseAIController::UpdateAI,
		SafeUpdateInterval,
		true,
		FirstDelay
	);
}

void ABaseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AIUpdateTimer);
	CachedPlayerCharacter.Reset();
	Super::EndPlay(EndPlayReason);
}

AMy* ABaseAIController::GetPlayerCharacter()
{
	if (!CachedPlayerCharacter.IsValid())
	{
		CachedPlayerCharacter = Cast<AMy>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	}

	return CachedPlayerCharacter.Get();
}

void ABaseAIController::UpdateAI()
{
	AHostile* Hostile = Cast<AHostile>(GetPawn());
	AMy* Player = GetPlayerCharacter();

	if (!Hostile || !Player)
	{
		EnterIdleState();
		return;
	}

	const float DistanceSq = FVector::DistSquared(
		Hostile->GetActorLocation(),
		Player->GetActorLocation()
	);

	if (DistanceSq > FMath::Square(FMath::Max(0.0f, DetectRange)))
	{
		EnterIdleState();
		return;
	}

	if (DistanceSq <= FMath::Square(FMath::Max(0.0f, AttackRange)))
	{
		EnterAttackState(Player);

		if (Hostile->CanAttack())
		{
			Hostile->Attack();
		}
		return;
	}

	UpdateChaseState(Player);
}

void ABaseAIController::EnterIdleState()
{
	if (CurrentState == ESimpleAIState::Idle)
	{
		return;
	}

	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	CurrentState = ESimpleAIState::Idle;
	LastMoveRequestLocation = FVector::ZeroVector;
	LastMoveRequestTime = -10000.0f;
}

void ABaseAIController::EnterAttackState(AMy* Player)
{
	if (!Player)
	{
		EnterIdleState();
		return;
	}

	SetFocus(Player);

	if (CurrentState != ESimpleAIState::Attack)
	{
		StopMovement();
		CurrentState = ESimpleAIState::Attack;
	}
}

void ABaseAIController::UpdateChaseState(AMy* Player)
{
	if (!Player)
	{
		EnterIdleState();
		return;
	}

	SetFocus(Player);

	const bool bEnteringChase = CurrentState != ESimpleAIState::Chase;
	CurrentState = ESimpleAIState::Chase;
	RequestMoveToPlayer(Player, bEnteringChase);
}

void ABaseAIController::RequestMoveToPlayer(AMy* Player, bool bForceRequest)
{
	if (!Player || !GetWorld())
	{
		return;
	}

	const FVector GoalLocation = Player->GetActorLocation();
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const bool bMoveInactive = GetMoveStatus() != EPathFollowingStatus::Moving;
	const bool bGoalMovedEnough = FVector::DistSquared(GoalLocation, LastMoveRequestLocation) >=
		FMath::Square(FMath::Max(1.0f, RepathDistanceThreshold));
	const bool bRepathCooldownElapsed = CurrentTime - LastMoveRequestTime >= FMath::Max(0.05f, MinRepathInterval);

	if (!bForceRequest && !bMoveInactive && !(bGoalMovedEnough && bRepathCooldownElapsed))
	{
		return;
	}

	MoveToActor(Player, MoveAcceptanceRadius);
	LastMoveRequestLocation = GoalLocation;
	LastMoveRequestTime = CurrentTime;
}
