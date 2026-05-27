#pragma once

#include "CoreMinimal.h"
#include "Attackif/Attackif.h"
#include "Attackif/AttackScore.h"

struct FAttackAccelerationConfig
{
    // 用最近几个原始输入帧判断是否正在加速。
    // 4 帧可以识别类似 10,30,50,90；过长会让触发滞后。
    int32 WindowSampleCount = 4;

    // 极小移动过滤，只用于抗抖，不作为速度阈值。
    float MinTerminalMove = 0.25f;
    float MinTotalMove = 4.0f;

    // 归一化增长判断，降低 DPI 和帧率差异的影响。
    float RelativeGrowthEpsilon = 0.06f;
    int32 MinPositiveGrowthCount = 2;
    float MinPositiveGrowthRatio = 0.65f;
    float MinNormalizedSlope = 0.10f;
    float MinLastMovePeakRatio = 0.82f;

    // 最后一段仍需继续增长；平台期不算挥刀加速。
    float MinTailGrowthRatio = 0.035f;

    // 最终加速度置信度阈值。
    float MinAccelerationScore = 0.55f;

    // 两次输入间隔过长时，之前的加速度窗口作废。
    float MaxFrameGapSeconds = 0.18f;
};

struct FAttackAccelerationResult
{
    bool bAccelerating = false;
    float Score = 0.0f;

    float NormalizedSlope = 0.0f;
    float PositiveGrowthRatio = 0.0f;
    int32 PositiveGrowthCount = 0;

    float LastMove = 0.0f;
    float MaxMove = 0.0f;
    float TailGrowth = 0.0f;
};

struct FAttackValidConfig
{
    FTrackDetectConfig TrackConfig;
    FAttackAccelerationConfig AccelerationConfig;
    FAttackScoreConfig ScoreConfig;

    // 用于获取方向的额外最小轨迹长度。
    // 默认 0 表示跟随 MinSampleDistance 自动推导，避免固定像素长度带来的 DPI 差异。
    float MinDirectionSegmentLength = 0.0f;
};

struct FAttackValidResult
{
    bool bCanTriggerAttack = false;
    bool bTrajectoryValid = false;
    bool bAccelerationValid = false;

    EAttackDirection Direction = EAttackDirection::None;
    float TrackScore = 0.0f;

    FTrajectoryResult Trajectory;
    FAttackAccelerationResult Acceleration;
};

class FAttackValid
{
public:
    FAttackValid();

    void Reset();

    bool PushInput(
        const FVector2D& Input,
        float CurrentTime,
        float MinSampleDistance,
        FAttackValidResult& OutResult);

    FAttackValidConfig& GetMutableConfig() { return Config; }
    const FAttackValidConfig& GetConfig() const { return Config; }

private:
    struct FMotionFrame
    {
        float MoveDistance = 0.0f;
        float TimeSeconds = 0.0f;
        float DeltaSeconds = 0.0f;
    };

    void PushMotionSample(
        const FVector2D& Input,
        float CurrentTime,
        float PreviousTime);

    FAttackAccelerationResult AnalyzeAccelerationIntent() const;

private:
    FAttackValidConfig Config;
    FTrackInputSampler Sampler;
    TArray<FMotionFrame> MotionFrames;
    float LastInputTime = -1.0f;
};
