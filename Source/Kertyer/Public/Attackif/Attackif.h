#pragma once
#include "DataAsset/AttackDH.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"
// 带时间戳的轨迹采样点
struct FTrackSample
{
    FVector2D Position = FVector2D::ZeroVector;
    float TimeSeconds = 0.0f;
};
// 轨迹优化配置
struct FTrackDetectConfig
{
    float PointMergeDistance = 2.0f;          // 最小相隔距离
    float ResampleDistance = 8.0f;            // 每隔多少距离重采样一个点
    int32 SmoothWindowSize = 5;               // 平滑时一次要看多少个相邻点
    float SmoothSigma = 1.2f;                 // 高斯标准差
    float SmoothStrength = 0.7f;              // 平滑混合强度 0~1
    float SharpAngleThresholdDeg = 40.0f;     // 尖角判定阈值（度）
    float EndPointRelativeThreshold = 0.06f;  // 终点补齐相对阈值（× ResampleDistance）
    float MinAbsEndThreshold = 0.05f;         // 终点补齐最小绝对阈值
    // 攻击触发阈值
    float MinValidSegmentLength = 280.0f;  // 最小有效线段长度
    float MinSwingSpeed = 1800.0f;         // 最低挥动速度（像素/秒）
    float SpeedWindowSeconds = 0.08f;      // 触发前短时间速度窗口
    float DirectionWeight = 0.25f;         // 方向权重
    float SpeedWeight = 0.75f;             // 速度权重
};
// 分数及有效线段
struct FTrackResult
{
    EAttackDirection Direction = EAttackDirection::None;
    // 轨迹分数
    float TrackScore = 0.0f;
    // 速度
    float SwingSpeed = 0.0f;
    // 速度分数
    float SpeedScore = 0.0f;
    // 速度是否达标
    bool bSpeedValid = false;
    // 是否可触发攻击
    bool bCanTriggerAttack = false;
    // 轨迹是否有效
    bool bValid = false;
    // 起始位置
    FVector2D Start = FVector2D::ZeroVector;
    // 终止位置
    FVector2D End = FVector2D::ZeroVector;
    // 有效线段长度
    float PathLength = 0.0f;
};
class FTrackPreprocessUtils
{
public:
    // 总调度：预处理轨迹、计算几何特征、计算速度并汇总最终结果
    // 1. 预处理轨迹（几何修正、重采样、平滑，并维护时序锚点）
    // 2. 计算处理后轨迹的几何特征（仅几何）
    // 3. 计算原始采样的速度
    // 4. 汇总方向、得分与可触发攻击标记
    static FTrackResult Transfer(
        const TArray<FTrackSample>& RawSamples,
        const FTrackDetectConfig& Config);
    // 预处理轨迹点
    // 注意：输出数组与输入数组不保证一一对应。
    // 该过程会进行近点合并、重采样、尾部处理和平滑，
    // 并同步维护每个点的 TimeSeconds 作为时序锚点/插值时序信息。
    // 这里的 TimeSeconds 不等价于平滑后 Position 的真实物理采样时刻，
    // 因此预处理结果不能用于速度、加速度或其他依赖真实采样时间的动态特征计算。
    static TArray<FTrackSample> PreprocessTrajectory(
        const TArray<FTrackSample>& RawPoints,
        const FTrackDetectConfig& Config);
    // 计算轨迹几何特征
    // 这里只做几何判定，不参与速度、时间窗口或总分逻辑。
    // 禁止在此函数内引入基于 TimeSeconds 的判定。
    static FTrackResult CalculateTrajectoryFeatures(
        const TArray<FTrackSample>& Samples,
        const FTrackDetectConfig& Config);
    // 计算方向
    static EAttackDirection Direction(const FTrackResult& Segment);
private:
    // 按最近时间窗口计算挥动速度
    // 速度必须始终基于原始采样点计算，不能使用预处理后的点。
    static float CalculateRecentSwingSpeed(
        const TArray<FTrackSample>& RawSamples,
        float WindowSeconds);
    // 速度归一化得分
    static float NormalizeSpeedScore(
        float Speed,
        float MinSwingSpeed);
    // 在两个带时间的采样点之间做同步插值
    // 新增点的 TimeSeconds 仅用于时序对齐，不代表后续平滑后位置的真实采样时刻。
    static FTrackSample LerpSample(
        const FTrackSample& A,
        const FTrackSample& B,
        float Alpha);
    // 计算两个采样点之间的位置距离
    static float DistanceBetweenSamples(
        const FTrackSample& A,
        const FTrackSample& B);
};







