#include "Combat/CombatSampling.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Combat/CombatStatusSwitch.h"
#include "DataAsset/AttackMoveDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void FCombatSampling::BeginAttackSampling(
	AActor* Owner,
	FCombatStatusSwitch& Status,
	float CurrentTime
)
{
	Status.RefreshAttackState(CurrentTime);

	if (Status.bIsAttackKeyDown)
	{
		return;
	}

	if (Status.IsLockedState())
	{
		return;
	}

	if (!FAttackAnimationPlayer::ResolveAnimInstance(Owner))
	{
		return;
	}

	Status.ResetAcceptedInput();
	Status.bIsAttackKeyDown = true;

	ClearSamplingBuffer();
	Status.RefreshAttackState(CurrentTime);
}

void FCombatSampling::EndAttackSampling(UWorld* World, FCombatStatusSwitch& Status)
{
	Status.bIsAttackKeyDown = false;
	Status.ResetAcceptedInput();

	ClearSamplingBuffer();

	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	Status.RefreshAttackState(CurrentTime);
}

bool FCombatSampling::CacheMouseInput(
	const FVector2D& Input,
	float CurrentTime,
	FCombatStatusSwitch& Status,
	EAttackDirection& OutDirection,
	float& OutTrackScore
)
{
	OutDirection = EAttackDirection::None;
	OutTrackScore = 0.0f;

	if (!Status.IsSamplingState())
	{
		return false;
	}

	FAttackValidResult Result;
	if (!AttackValid.PushInput(Input, CurrentTime, MinSampleDistance, Result))
	{
		return false;
	}

	if (!Result.bCanTriggerAttack || Result.Direction == EAttackDirection::None)
	{
		return false;
	}

	if (!Status.CanAcceptAttackInput(Result.Direction, CurrentTime))
	{
		ClearSamplingBuffer();
		return false;
	}

	Status.MarkAttackInputAccepted(Result.Direction, CurrentTime);

	OutDirection = Result.Direction;
	OutTrackScore = Result.TrackScore;

	return true;
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

void FCombatSampling::QueuePendingAttack(EAttackDirection Direction, float TrackScore)
{
	bHasPendingAttack = Direction != EAttackDirection::None;
	PendingDirection = Direction;
	PendingTrackScore = bHasPendingAttack ? TrackScore : 0.0f;
}

const FAttackMoveData* FCombatSampling::FindAttackMoveByDirection(
	const UAttackMoveDataAsset* AttackMoveDataAsset,
	EAttackDirection InDirection
) const
{
	if (!AttackMoveDataAsset || InDirection == EAttackDirection::None)
	{
		return nullptr;
	}

	return AttackMoveDataAsset->FindAttackByDirection(InDirection);
}