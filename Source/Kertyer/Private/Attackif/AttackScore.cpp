#include "Attackif/AttackScore.h"


float AttackScore::CalculateTrackScore(
    const FTrajectoryResult& Trajectory,
    float AccelerationScore,
    const FAttackScoreConfig& Config)
{
    if (!Trajectory.bValid || Trajectory.Direction == EAttackDirection::None)
    {
        return 0.0f;
    }

    const float SafeDirectionWeight = FMath::Max(0.0f, Config.DirectionWeight);
    const float SafeAccelerationWeight = FMath::Max(0.0f, Config.AccelerationWeight);
    const float WeightSum = SafeDirectionWeight + SafeAccelerationWeight;

    if (WeightSum <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    const float DirectionScore = 1.0f;
    const float SafeAccelerationScore = Normalize01(AccelerationScore);

    const float Score =
        (DirectionScore * SafeDirectionWeight + SafeAccelerationScore * SafeAccelerationWeight) / WeightSum;

    return FMath::Clamp(Score, FMath::Clamp(Config.MinAcceptedScore, 0.0f, 1.0f), 1.0f);
}

float AttackScore::Normalize01(float Value)
{
    return FMath::Clamp(Value, 0.0f, 1.0f);
}
