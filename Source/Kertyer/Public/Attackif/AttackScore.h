#pragma once

#include "CoreMinimal.h"
#include "Attackif/Attackif.h"

// 攻击评分配置。
// 评分不负责判断能不能攻击，只负责把有效轨迹与加速度强度换算成伤害倍率/评分。
struct FAttackScoreConfig
{
    float DirectionWeight = 0.35f;
    float AccelerationWeight = 0.65f;

    // 攻击已经被 AttackValid 判定可触发时，最低保底分。
    // 避免刚达标的攻击把下一击伤害倍率压得过低。
    float MinAcceptedScore = 0.50f;
};

class KERTYER_API AttackScore
{
public:
    AttackScore();
    ~AttackScore();

    static float CalculateTrackScore(
        const FTrajectoryResult& Trajectory,
        float AccelerationScore,
        const FAttackScoreConfig& Config = FAttackScoreConfig());

    static float Normalize01(float Value);
};
