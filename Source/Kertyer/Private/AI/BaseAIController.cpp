#include "AI/BaseAIController.h"

#include "Character/Hostile.h"
#include "Character/My.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void ABaseAIController::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		AIUpdateTimer,
		this,
		&ABaseAIController::UpdateAI,
		UpdateInterval,
		true
	);
}

AMy* ABaseAIController::GetPlayerCharacter() const
{
	return Cast<AMy>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
}

void ABaseAIController::UpdateAI()
{
	AHostile* Hostile = Cast<AHostile>(GetPawn());
	AMy* Player = GetPlayerCharacter();

	if (!Hostile || !Player)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist(
		Hostile->GetActorLocation(),
		Player->GetActorLocation()
	);

	if (DistanceToPlayer > DetectRange)
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	SetFocus(Player);

	if (DistanceToPlayer <= AttackRange)
	{
		StopMovement();

		if (Hostile->CanAttack())
		{
			Hostile->Attack();
		}

		return;
	}

	MoveToActor(Player, MoveAcceptanceRadius);
}

