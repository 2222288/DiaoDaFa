#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"

// 带时间戳的轨迹采样点。
// Position 是从本次攻击采样开始后累计出的鼠标轨迹坐标，不是屏幕绝对坐标。
struct FTrackSample
{
    FVector2D Position = FVector2D::ZeroVector;
    float TimeSeconds = 0.0f;
};

// 轨迹处理配置。
// 这里只保留几何轨迹相关参数；攻击触发、加速度、分数不放在这里。
struct FTrackDetectConfig
{
    float PointMergeDistance = 2.0f;          // 最小合并距离，过滤过密点
    float ResampleDistance = 8.0f;            // 重采样距离
    int32 SmoothWindowSize = 5;               // 平滑窗口
    float SmoothSigma = 1.2f;                 // 高斯标准差
    float SmoothStrength = 0.7f;              // 平滑混合强度 0~1
    float SharpAngleThresholdDeg = 40.0f;     // 尖角保护阈值
    float EndPointRelativeThreshold = 0.06f;  // 预留：终点补齐相对阈值
    float MinAbsEndThreshold = 0.05f;         // 预留：终点补齐最小绝对阈值

    // 轨迹几何有效所需的最小线段长度。
    // 该值只用于“方向/轨迹是否可靠”，不再作为攻击触发的速度条件。
    float MinValidSegmentLength = 280.0f;
};

// 纯轨迹结果：只描述轨迹是否有效、方向、起止点、长度和处理后采样。
struct FTrackResult
{
    bool bValid = false;
    EAttackDirection Direction = EAttackDirection::None;

    FVector2D Start = FVector2D::ZeroVector;
    FVector2D End = FVector2D::ZeroVector;
    float PathLength = 0.0f;

    TArray<FTrackSample> ProcessedSamples;
};

// 兼容旧命名：AttackValid 以及旧代码可继续使用 FTrajectoryResult。
using FTrajectoryResult = FTrackResult;

// 鼠标输入到轨迹采样的缓存器。
// AttackComponent 不再自己维护 RawPoints/AccumulatedMousePosition。
class FTrackInputSampler
{
public:
    void Reset();

    // Input 是本帧鼠标增量；内部会累计为本次攻击的局部轨迹坐标。
    // 返回值表示本帧是否追加了新的轨迹采样点。
    bool PushInput(
        const FVector2D& Input,
        float CurrentTime,
        float MinSampleDistance);

    const TArray<FTrackSample>& GetSamples() const { return TrackSamples; }
    const FVector2D& GetAccumulatedPosition() const { return AccumulatedPosition; }
    int32 Num() const { return TrackSamples.Num(); }

private:
    TArray<FTrackSample> TrackSamples;
    FVector2D AccumulatedPosition = FVector2D::ZeroVector;
};

class FTrackPreprocessUtils
{
public:
    // 总调度：只做轨迹分析。
    // 1. 预处理轨迹：合并、重采样、尾部保护、平滑。
    // 2. 计算几何有效线段。
    // 3. 根据有效线段返回方向。
    static FTrajectoryResult AnalyzeTrajectory(
        const TArray<FTrackSample>& RawSamples,
        const FTrackDetectConfig& Config);

    // 兼容旧接口名。现在只等价于 AnalyzeTrajectory，不再计算速度、得分或触发状态。
    static FTrajectoryResult Transfer(
        const TArray<FTrackSample>& RawSamples,
        const FTrackDetectConfig& Config);

    // 预处理轨迹点。
    // 输出数组与输入数组不保证一一对应。
    // TimeSeconds 只作为插值锚点，不能用于速度、加速度等真实动态特征计算。
    static TArray<FTrackSample> PreprocessTrajectory(
        const TArray<FTrackSample>& RawPoints,
        const FTrackDetectConfig& Config);

    // 计算轨迹几何特征。
    // 这里只做几何判定，不参与速度、加速度、分数或触发逻辑。
    static FTrajectoryResult CalculateTrajectoryFeatures(
        const TArray<FTrackSample>& Samples,
        const FTrackDetectConfig& Config);

    static EAttackDirection Direction(const FTrajectoryResult& Segment);

private:
    static FTrackSample LerpSample(
        const FTrackSample& A,
        const FTrackSample& B,
        float Alpha);

    static float DistanceBetweenSamples(
        const FTrackSample& A,
        const FTrackSample& B);
};
