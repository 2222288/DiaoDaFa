#include "Attackif/AttackValid.h"

FAttackValid::FAttackValid()
{
    Config.TrackConfig.MinValidSegmentLength = Config.MinDirectionSegmentLength;
}

void FAttackValid::Reset()
{
    Sampler.Reset();
    MotionFrames.Empty();
    LastInputTime = -1.0f;
}

bool FAttackValid::PushInput(
    const FVector2D& Input,
    float CurrentTime,
    float MinSampleDistance,
    FAttackValidResult& OutResult)
{
    OutResult = FAttackValidResult();

    const float PreviousInputTime = LastInputTime;

    if (PreviousInputTime > 0.0f &&
        CurrentTime - PreviousInputTime > Config.AccelerationConfig.MaxFrameGapSeconds)
    {
        MotionFrames.Empty();
    }

    PushMotionSample(Input, CurrentTime, PreviousInputTime);
    LastInputTime = CurrentTime;

    Sampler.PushInput(Input, CurrentTime, MinSampleDistance);

    FTrackDetectConfig LocalTrackConfig = Config.TrackConfig;
    LocalTrackConfig.MinValidSegmentLength = FMath::Max(
        FMath::Max(0.0f, Config.MinDirectionSegmentLength),
        FMath::Max(0.0f, MinSampleDistance) * 2.0f);

    OutResult.Trajectory = FTrackPreprocessUtils::AnalyzeTrajectory(
        Sampler.GetSamples(),
        LocalTrackConfig);

    OutResult.Direction = OutResult.Trajectory.Direction;
    OutResult.bTrajectoryValid =
        OutResult.Trajectory.bValid &&
        OutResult.Direction != EAttackDirection::None;

    OutResult.Acceleration = AnalyzeAccelerationIntent();
    OutResult.bAccelerationValid = OutResult.Acceleration.bAccelerating;

    if (!OutResult.bTrajectoryValid || !OutResult.bAccelerationValid)
    {
        return false;
    }

    OutResult.TrackScore = AttackScore::CalculateTrackScore(
        OutResult.Trajectory,
        OutResult.Acceleration.Score,
        Config.ScoreConfig);

    OutResult.bCanTriggerAttack = OutResult.TrackScore > 0.0f;
    return OutResult.bCanTriggerAttack;
}

void FAttackValid::PushMotionSample(
    const FVector2D& Input,
    float CurrentTime,
    float PreviousTime)
{
    FMotionFrame Frame;
    Frame.MoveDistance = Input.Size();
    Frame.TimeSeconds = CurrentTime;
    Frame.DeltaSeconds = PreviousTime > 0.0f ? FMath::Max(0.0f, CurrentTime - PreviousTime) : 0.0f;
    MotionFrames.Add(Frame);

    const int32 MaxHistoryCount = FMath::Max(12, Config.AccelerationConfig.WindowSampleCount + 4);
    while (MotionFrames.Num() > MaxHistoryCount)
    {
        MotionFrames.RemoveAt(0);
    }
}

FAttackAccelerationResult FAttackValid::AnalyzeAccelerationIntent() const
{
    FAttackAccelerationResult Result;

    const int32 WindowCount = FMath::Clamp(Config.AccelerationConfig.WindowSampleCount, 4, 12);
    if (MotionFrames.Num() < WindowCount)
    {
        return Result;
    }

    TArray<float> Moves;
    Moves.Reserve(WindowCount);

    const int32 StartIndex = MotionFrames.Num() - WindowCount;
    float RawTotalMove = 0.0f;
    float RawLastMove = 0.0f;
    float MaxMove = 0.0f;

    for (int32 i = StartIndex; i < MotionFrames.Num(); ++i)
    {
        const float RawMove = FMath::Max(0.0f, MotionFrames[i].MoveDistance);
        RawTotalMove += RawMove;
        RawLastMove = RawMove;

        // 用单位时间移动量参与“是否在加速”的形态判断。
        // 后续全部做归一化，不再把绝对速度当触发阈值。
        const float SafeDeltaSeconds = MotionFrames[i].DeltaSeconds > KINDA_SMALL_NUMBER
            ? FMath::Clamp(MotionFrames[i].DeltaSeconds, 1.0f / 240.0f, 1.0f / 15.0f)
            : 1.0f / 60.0f;

        const float Move = RawMove / SafeDeltaSeconds;
        Moves.Add(Move);
        MaxMove = FMath::Max(MaxMove, Move);
    }

    Result.LastMove = Moves.Last();
    Result.MaxMove = MaxMove;

    if (RawLastMove < Config.AccelerationConfig.MinTerminalMove ||
        RawTotalMove < Config.AccelerationConfig.MinTotalMove ||
        MaxMove <= KINDA_SMALL_NUMBER)
    {
        return Result;
    }

    const float GrowthEpsilon = FMath::Max(
        0.15f,
        MaxMove * FMath::Clamp(Config.AccelerationConfig.RelativeGrowthEpsilon, 0.0f, 0.5f));

    float PositiveGrowthEnergy = 0.0f;
    float NegativeGrowthEnergy = 0.0f;
    int32 PositiveGrowthCount = 0;

    for (int32 i = 1; i < Moves.Num(); ++i)
    {
        const float Delta = Moves[i] - Moves[i - 1];
        if (Delta > GrowthEpsilon)
        {
            PositiveGrowthEnergy += Delta;
            ++PositiveGrowthCount;
        }
        else if (Delta < -GrowthEpsilon)
        {
            NegativeGrowthEnergy += -Delta;
        }
    }

    const float GrowthEnergySum = PositiveGrowthEnergy + NegativeGrowthEnergy;
    const float PositiveGrowthRatio =
        GrowthEnergySum > KINDA_SMALL_NUMBER
            ? PositiveGrowthEnergy / GrowthEnergySum
            : 0.0f;

    Result.PositiveGrowthCount = PositiveGrowthCount;
    Result.PositiveGrowthRatio = PositiveGrowthRatio;

    // 对移动量做 0~1 归一化后算线性斜率。
    // 只看增长形态，不直接依赖绝对像素速度。
    const float MeanX = static_cast<float>(Moves.Num() - 1) * 0.5f;
    float VarX = 0.0f;
    float CovXY = 0.0f;
    float MeanY = 0.0f;

    for (const float Move : Moves)
    {
        MeanY += Move / MaxMove;
    }
    MeanY /= static_cast<float>(Moves.Num());

    for (int32 i = 0; i < Moves.Num(); ++i)
    {
        const float X = static_cast<float>(i) - MeanX;
        const float Y = Moves[i] / MaxMove - MeanY;
        VarX += X * X;
        CovXY += X * Y;
    }

    const float NormalizedSlope = VarX > KINDA_SMALL_NUMBER ? CovXY / VarX : 0.0f;
    Result.NormalizedSlope = NormalizedSlope;

    const float TailGrowth = Moves.Last() - Moves[Moves.Num() - 2];
    Result.TailGrowth = TailGrowth;

    const bool bEnoughPositiveGrowth =
        PositiveGrowthCount >= Config.AccelerationConfig.MinPositiveGrowthCount &&
        PositiveGrowthRatio >= Config.AccelerationConfig.MinPositiveGrowthRatio;

    const bool bSlopeValid =
        NormalizedSlope >= Config.AccelerationConfig.MinNormalizedSlope;

    const bool bLastNearPeak =
        Moves.Last() >= MaxMove * FMath::Clamp(Config.AccelerationConfig.MinLastMovePeakRatio, 0.0f, 1.0f);

    const bool bTailStillGrowing =
        TailGrowth > GrowthEpsilon ||
        TailGrowth > MaxMove * FMath::Max(0.0f, Config.AccelerationConfig.MinTailGrowthRatio);

    const float SlopeScore = FMath::Clamp(
        (NormalizedSlope - Config.AccelerationConfig.MinNormalizedSlope) / 0.30f,
        0.0f,
        1.0f);

    const float TailScore = FMath::Clamp(
        TailGrowth / FMath::Max(MaxMove * 0.35f, KINDA_SMALL_NUMBER),
        0.0f,
        1.0f);

    const int32 HalfCount = FMath::Max(1, Moves.Num() / 2);
    float FirstHalfMean = 0.0f;
    for (int32 i = 0; i < HalfCount; ++i)
    {
        FirstHalfMean += Moves[i];
    }
    FirstHalfMean /= static_cast<float>(HalfCount);

    const float EndDominanceScore = FMath::Clamp(
        (Moves.Last() - FirstHalfMean) / MaxMove,
        0.0f,
        1.0f);

    Result.Score = FMath::Clamp(
        0.35f * SlopeScore +
        0.25f * PositiveGrowthRatio +
        0.25f * TailScore +
        0.15f * EndDominanceScore,
        0.0f,
        1.0f);

    Result.bAccelerating =
        bEnoughPositiveGrowth &&
        bSlopeValid &&
        bLastNearPeak &&
        bTailStillGrowing &&
        Result.Score >= Config.AccelerationConfig.MinAccelerationScore;

    return Result;
}
