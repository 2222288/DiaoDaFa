#include "Combat/CombatSampling.h"
#include "AnimationLogic/AttackAnimationPlayer.h"



void FCombatSampling::BeginAttackSampling(float CurrentTime)
{
	RefreshAttackState(CurrentTime);

	if (bIsAttackKeyDown)
	{
		return;
	}

	if (IsLockedState())
	{
		return;
	}

	if (!FAttackAnimationPlayer::ResolveAnimInstance(GetOwner()))
	{
		return;
	}

	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;
	bIsAttackKeyDown = true;

	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

void FCombatSampling::EndAttackSampling()
{
	bIsAttackKeyDown = false;
	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;

	ClearSamplingBuffer();

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

	RefreshAttackState(CurrentTime);
}

void FCombatSampling::CacheMouseInput(const FVector2D& Input, float CurrentTime)
{
	if (!IsSamplingState())
	{
		return;
	}

	FAttackValidResult Result;
	if (!AttackValid.PushInput(Input, CurrentTime, MinSampleDistance, Result))
	{
		return;
	}

	if (!Result.bCanTriggerAttack || Result.Direction == EAttackDirection::None)
	{
		return;
	}

	if (!CanAcceptAttackInput(Result.Direction, CurrentTime))
	{
		ClearSamplingBuffer();
		return;
	}

	MarkAttackInputAccepted(Result.Direction, CurrentTime);
	PerformAttack(Result.Direction, Result.TrackScore);
}

void FCombatSampling::ClearSamplingBuffer()
{
	AttackValid.Reset();
}

void FCombatSampling::ClearPendingAttack()
{
	bHasPendingAttack = false;
	PendingDirection = EAttackDirection::None;
	PendingTrackScore = 0.0f;
}

