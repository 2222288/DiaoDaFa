#pragma once

#include "CoreMinimal.h"
#include "Attackif/AttackValid.h"
#include "Combat/CombatTypes.h"

class FCombatStatusSwitch;
class UAttackMoveDataAsset;
struct FAttackMoveData;

/**
 * 战斗采样模块：
 * 只负责鼠标轨迹采样、攻击输入有效性判断和待定攻击缓存。
 */
class KERTYER_API FCombatSampling
{
public:
	void BeginAttackSampling(AActor* Owner, FCombatStatusSwitch& Status, float CurrentTime);

	void EndAttackSampling(UWorld* World, FCombatStatusSwitch& Status);

	bool CacheMouseInput(
		const FVector2D& Input,
		float CurrentTime,
		FCombatStatusSwitch& Status,
		EAttackDirection& OutDirection,
		float& OutTrackScore
	);

	void ClearSamplingBuffer();

	void ClearPendingAttack();

	void QueuePendingAttack(EAttackDirection Direction, float TrackScore);

	bool HasPendingAttack() const { return bHasPendingAttack; }

	EAttackDirection GetPendingDirection() const { return PendingDirection; }

	float GetPendingTrackScore() const { return PendingTrackScore; }

	const FAttackMoveData* FindAttackMoveByDirection(
		const UAttackMoveDataAsset* AttackMoveDataAsset,
		EAttackDirection InDirection
	) const;

public:
	float MinSampleDistance = 8.0f;

private:
	FAttackValid AttackValid;

	bool bHasPendingAttack = false;

	EAttackDirection PendingDirection = EAttackDirection::None;

	float PendingTrackScore = 0.0f;
};