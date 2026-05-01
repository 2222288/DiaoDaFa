
#include "Attackif/Attackif.h"
#include "GenericPlatform/GenericPlatformTime.h"
// 总调度函数
// 1. 预处理轨迹（几何修正、重采样、平滑，并维护时序锚点）
// 2. 计算处理后轨迹的几何特征（仅几何）
// 3. 计算原始采样的速度
// 4. 汇总方向、得分与可触发攻击标记
FTrackResult FTrackPreprocessUtils::Transfer(const TArray<FTrackSample>& RawSamples, const FTrackDetectConfig& Config)
{
    FTrackResult OutResult;
    if (RawSamples.Num() < 2)
    {
        return OutResult;
    }
    const TArray<FTrackSample> ProcessedPoints = PreprocessTrajectory(RawSamples, Config);
    OutResult = CalculateTrajectoryFeatures(ProcessedPoints, Config);
    OutResult.SwingSpeed = CalculateRecentSwingSpeed(RawSamples, Config.SpeedWindowSeconds);
    OutResult.bSpeedValid = OutResult.SwingSpeed >= Config.MinSwingSpeed;
    OutResult.SpeedScore = NormalizeSpeedScore(OutResult.SwingSpeed, Config.MinSwingSpeed);

    //方向分数
    const float DirectionScore = OutResult.bValid ? 1.0f : 0.0f;
    //方向权重
    const float DirectionWeight = FMath::Max(0.0f, Config.DirectionWeight);
    //速度权重
    const float SpeedWeight = FMath::Max(0.0f, Config.SpeedWeight);
	//权重总和
    const float WeightSum = DirectionWeight + SpeedWeight;
    //最终分数计算
    if (WeightSum > KINDA_SMALL_NUMBER)
    {
        OutResult.TrackScore = (DirectionScore * DirectionWeight + OutResult.SpeedScore * SpeedWeight) / WeightSum;
    }
    else
    {
        OutResult.TrackScore = 0.0f;
    }
    OutResult.Direction = Direction(OutResult);
    OutResult.bCanTriggerAttack = OutResult.bValid && OutResult.bSpeedValid && (OutResult.Direction != EAttackDirection::None);
    return OutResult;
}
// 预处理轨迹点
// 该函数只负责轨迹形状优化，不负责速度计算。
// 这里维护的 TimeSeconds 语义是“时序锚点”：
// - 新增点会用位置与时间同步插值生成一个合理的时序标签；
// - 平滑阶段只改 Position，不改 TimeSeconds；
// - 因此输出中的 TimeSeconds 不等价于平滑后 Position 的真实物理采样时刻。
// 任何速度、加速度、时间窗口等动态特征计算，必须基于原始采样点完成。
TArray<FTrackSample> FTrackPreprocessUtils::PreprocessTrajectory(const TArray<FTrackSample>& RawPoints, const FTrackDetectConfig& Config)
{
    if (RawPoints.Num() <= 0)
    {
        return {};
    }
    const float MergeDistance = FMath::Max(0.0f, Config.PointMergeDistance);
    const float MergeDistSq = FMath::Square(MergeDistance);
    const float ResampleDist = Config.ResampleDistance;
    if (RawPoints.Num() == 1 || ResampleDist <= KINDA_SMALL_NUMBER)
    {
        return RawPoints;
    }
    const auto SmoothStep01 = [](float T) -> float
        {
            T = FMath::Clamp(T, 0.0f, 1.0f);
            return T * T * (3.0f - 2.0f * T);
        };
    TArray<FTrackSample> Working;
    Working.Reserve(RawPoints.Num());
    Working.Add(RawPoints[0]);
    // 第一步：合并过近点，避免极高采样密度带来的抖动。
    for (int32 i = 1; i < RawPoints.Num(); ++i)
    {
        const FTrackSample& Candidate = RawPoints[i];
        if ((Candidate.Position - Working.Last().Position).SizeSquared() > MergeDistSq)
        {
            Working.Add(Candidate);
        }
        else
        {
            Working.Last() = Candidate;
        }
    }
    if (Working.Num() <= 2)
    {
        return Working;
    }
    const FTrackSample LatestObservedPoint = Working.Last();
    const float PointDedupEpsUpper = FMath::Max(KINDA_SMALL_NUMBER, 0.02f * ResampleDist);
    const float PointDedupEps = FMath::Clamp(
        FMath::Max(ResampleDist * 1e-3f, 0.25f * MergeDistance),
        KINDA_SMALL_NUMBER,
        PointDedupEpsUpper);
    const float PointDedupEpsSq = FMath::Square(PointDedupEps);
    const float ArcLengthEps = FMath::Max(KINDA_SMALL_NUMBER, ResampleDist * 1e-4f);
    float RecentMeanSegLen = 0.0f;
    int32 RecentSegCount = 0;
    TArray<FVector2D> RecentNormals;
    RecentNormals.Reserve(5);
    const int32 RecentStartIndex = FMath::Max(1, Working.Num() - 5);
    for (int32 i = RecentStartIndex; i < Working.Num(); ++i)
    {
        const FVector2D Seg = Working[i].Position - Working[i - 1].Position;
        const float Len = Seg.Size();
        if (Len <= KINDA_SMALL_NUMBER)
        {
            continue;
        }
        RecentMeanSegLen += Len;
        ++RecentSegCount;
        RecentNormals.Add(Seg / Len);
    }
    if (RecentSegCount > 0)
    {
        RecentMeanSegLen /= static_cast<float>(RecentSegCount);
    }
    else
    {
        RecentMeanSegLen = ResampleDist;
    }
    float TailDirectionConsistency01 = 1.0f;
    if (RecentNormals.Num() >= 2)
    {
        float DotAccum = 0.0f;
        float MinDot01 = 1.0f;
        float LastDot01 = 1.0f;
        for (int32 i = 1; i < RecentNormals.Num(); ++i)
        {
            const float Dot = FVector2D::DotProduct(RecentNormals[i - 1], RecentNormals[i]);
            const float Dot01 = FMath::Clamp(0.5f * (Dot + 1.0f), 0.0f, 1.0f);
            DotAccum += Dot01;
            MinDot01 = FMath::Min(MinDot01, Dot01);
            LastDot01 = Dot01;
        }
        const float MeanDot01 = DotAccum / static_cast<float>(RecentNormals.Num() - 1);
        TailDirectionConsistency01 = FMath::Clamp(
            0.55f * MeanDot01 + 0.25f * MinDot01 + 0.20f * LastDot01,
            0.0f,
            1.0f);
    }
    const float SpeedScale = FMath::Clamp(RecentMeanSegLen / FMath::Max(ResampleDist, KINDA_SMALL_NUMBER), 0.0f, 1.35f);
    const float JitterScale = 1.0f - TailDirectionConsistency01;
    const float UnstableTailArc = FMath::Clamp(
        ResampleDist * (1.00f + 0.75f * SpeedScale + 0.95f * JitterScale),
        0.85f * ResampleDist,
        3.60f * ResampleDist);
    int32 FirstUnstableWorkingIndex = Working.Num() - 1;
    float WorkingTailArcAccum = 0.0f;
    while (FirstUnstableWorkingIndex > 1)
    {
        const float SegLen = DistanceBetweenSamples(
            Working[FirstUnstableWorkingIndex],
            Working[FirstUnstableWorkingIndex - 1]);
        if (WorkingTailArcAccum + SegLen > UnstableTailArc && WorkingTailArcAccum > KINDA_SMALL_NUMBER)
        {
            break;
        }
        WorkingTailArcAccum += SegLen;
        --FirstUnstableWorkingIndex;
    }
    const int32 ArcTailPointCount = FMath::Max(0, Working.Num() - FirstUnstableWorkingIndex);
    const float NominalTailSegLen = FMath::Max(RecentMeanSegLen, FMath::Max(0.30f * ResampleDist, PointDedupEps));
    const float JitterTail01 = SmoothStep01(FMath::Clamp((JitterScale - 0.08f) / 0.55f, 0.0f, 1.0f));
    const float LowSpeedTail01 = 1.0f - SmoothStep01(FMath::Clamp((SpeedScale - 0.10f) / 0.95f, 0.0f, 1.0f));
    const float TailBudget01 = FMath::Clamp(0.62f * JitterTail01 + 0.38f * LowSpeedTail01, 0.0f, 1.0f);
    const int32 BudgetTailPointCount = FMath::Clamp(FMath::RoundToInt(FMath::Lerp(0.0f, 3.0f, TailBudget01)), 0, 3);
    const float TailBudgetArc = UnstableTailArc * FMath::Lerp(0.82f, 1.18f, TailBudget01);
    const int32 BudgetArcPointCount = FMath::Clamp(
        FMath::CeilToInt(TailBudgetArc / FMath::Max(0.60f * ResampleDist, NominalTailSegLen)),
        0,
        4);
    int32 TailPointCount = FMath::Max(ArcTailPointCount, FMath::Max(BudgetTailPointCount, BudgetArcPointCount));
    TailPointCount = FMath::Clamp(TailPointCount, 0, FMath::Max(0, Working.Num() - 2));
    const int32 StableInputEndIndex = FMath::Clamp(Working.Num() - 1 - TailPointCount, 1, Working.Num() - 1);
    const int32 FirstTailWorkingIndex = StableInputEndIndex + 1;
    TArray<float> StableInputArcs;
    StableInputArcs.SetNumUninitialized(StableInputEndIndex + 1);
    StableInputArcs[0] = 0.0f;
    for (int32 i = 1; i <= StableInputEndIndex; ++i)
    {
        StableInputArcs[i] = StableInputArcs[i - 1] + DistanceBetweenSamples(Working[i - 1], Working[i]);
    }
    const float StableInputLength = StableInputArcs.Last();
    if (StableInputLength <= KINDA_SMALL_NUMBER)
    {
        return Working;
    }
    const int32 EstStableCount = FMath::CeilToInt(StableInputLength / ResampleDist) + 4;
    TArray<FTrackSample> StableOutput;
    TArray<float> StableOutputArcs;
    StableOutput.Reserve(FMath::Max(StableInputEndIndex + 1, EstStableCount));
    StableOutputArcs.Reserve(FMath::Max(StableInputEndIndex + 1, EstStableCount));
    StableOutput.Add(Working[0]);
    StableOutputArcs.Add(0.0f);
    // 第二步：对稳定段进行重采样。
    // 这里通过 LerpSample 同时插值 Position 和 TimeSeconds，
    // 为新增点生成同步时序锚点；该时间仅用于时序对齐，
    // 不代表后续平滑后位置的真实物理采样时刻。
    float NextSampleAt = ResampleDist;
    for (int32 i = 0; i < StableInputEndIndex; ++i)
    {
        const FTrackSample& A = Working[i];
        const FTrackSample& B = Working[i + 1];
        const float SegLen = DistanceBetweenSamples(A, B);
        if (SegLen <= KINDA_SMALL_NUMBER)
        {
            continue;
        }
        const float SegmentStartArc = StableInputArcs[i];
        const float SegmentEndArc = StableInputArcs[i + 1];
        while (NextSampleAt <= SegmentEndArc + ArcLengthEps)
        {
            const float LocalDist = NextSampleAt - SegmentStartArc;
            const float T = FMath::Clamp(LocalDist / SegLen, 0.0f, 1.0f);
            const FTrackSample P = LerpSample(A, B, T);
            if ((P.Position - StableOutput.Last().Position).SizeSquared() > PointDedupEpsSq)
            {
                StableOutput.Add(P);
                StableOutputArcs.Add(NextSampleAt);
            }
            NextSampleAt += ResampleDist;
        }
    }
    const FTrackSample& StableEndPoint = Working[StableInputEndIndex];
    if ((StableEndPoint.Position - StableOutput.Last().Position).SizeSquared() > PointDedupEpsSq)
    {
        StableOutput.Add(StableEndPoint);
        StableOutputArcs.Add(StableInputLength);
    }
    else
    {
        StableOutput.Last() = StableEndPoint;
        StableOutputArcs.Last() = StableInputLength;
    }
    const float TailSpacingBase = ResampleDist * FMath::Lerp(0.16f, 0.28f, TailDirectionConsistency01);
    const float TailSpeedScale = FMath::Lerp(0.90f, 1.10f, SpeedScale / 1.35f);
    const float TailMinSpacing = FMath::Max(PointDedupEps, TailSpacingBase * TailSpeedScale);
    const float TailNearEndMinSpacing = FMath::Max(PointDedupEps, 0.45f * TailMinSpacing);
    TArray<FTrackSample> Output = StableOutput;
    const int32 StableOutputEndIndex = Output.Num() - 1;
    const int32 TailWorkingCount = FMath::Max(0, Working.Num() - FirstTailWorkingIndex);
    Output.Reserve(Output.Num() + TailWorkingCount);
    float TailArcSinceEmit = 0.0f;
    FVector2D LastAcceptedTailDir(0.0f, 0.0f);
    bool bHasAcceptedTailDir = false;
    for (int32 i = FirstTailWorkingIndex; i < Working.Num(); ++i)
    {
        const FTrackSample& TailPoint = Working[i];
        const bool bIsLastTailPoint = (i == Working.Num() - 1);
        const int32 TailOrdinalFromEnd = (Working.Num() - 1) - i;
        const float RequiredSpacing = (TailOrdinalFromEnd <= 1) ? TailNearEndMinSpacing : TailMinSpacing;
        const float DistSq = (TailPoint.Position - Output.Last().Position).SizeSquared();
        if (i > FirstTailWorkingIndex)
        {
            TailArcSinceEmit += DistanceBetweenSamples(Working[i - 1], Working[i]);
        }
        else if (FirstTailWorkingIndex > 0)
        {
            TailArcSinceEmit += DistanceBetweenSamples(Working[FirstTailWorkingIndex - 1], Working[FirstTailWorkingIndex]);
        }
        float DirectionNovelty01 = 0.0f;
        const FVector2D CandidateDir = TailPoint.Position - Output.Last().Position;
        if (bHasAcceptedTailDir && CandidateDir.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            const float Dot01 = FMath::Clamp(0.5f * (FVector2D::DotProduct(CandidateDir.GetSafeNormal(), LastAcceptedTailDir) + 1.0f), 0.0f, 1.0f);
            DirectionNovelty01 = 1.0f - Dot01;
        }
        const bool bAllowBySpacing = DistSq > FMath::Square(RequiredSpacing);
        const bool bAllowByAccumulatedArc = TailArcSinceEmit > (0.72f * RequiredSpacing);
        const bool bAllowByDirectionChange = DirectionNovelty01 > 0.16f && DistSq > PointDedupEpsSq;
        const bool bAllowNearEndMicroMotion = (TailOrdinalFromEnd <= 1) && DistSq > PointDedupEpsSq;
        if (bAllowBySpacing || bAllowByAccumulatedArc || bAllowByDirectionChange || bAllowNearEndMicroMotion)
        {
            const FVector2D PrevAccepted = Output.Last().Position;
            Output.Add(TailPoint);
            TailArcSinceEmit = 0.0f;
            const FVector2D AcceptedDir = TailPoint.Position - PrevAccepted;
            if (AcceptedDir.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                LastAcceptedTailDir = AcceptedDir.GetSafeNormal();
                bHasAcceptedTailDir = true;
            }
        }
        else if (bIsLastTailPoint)
        {
            Output.Last() = TailPoint;
        }
    }
    if (Output.Num() <= 2)
    {
        Output.Last() = LatestObservedPoint;
        return Output;
    }
    struct FCornerMarker
    {
        float Arc = 0.0f;
        float Strength01 = 0.0f;
    };
    TArray<FCornerMarker> StableCorners;
    StableCorners.Reserve(StableInputEndIndex);
    if (Config.SharpAngleThresholdDeg > 0.1f && StableInputEndIndex >= 2)
    {
        const float Denom = FMath::Max(1.0f, 180.0f - Config.SharpAngleThresholdDeg);
        for (int32 i = 1; i < StableInputEndIndex; ++i)
        {
            const FVector2D Prev = Working[i].Position - Working[i - 1].Position;
            const FVector2D Next = Working[i + 1].Position - Working[i].Position;
            const float PrevLen = Prev.Size();
            const float NextLen = Next.Size();
            if (PrevLen <= KINDA_SMALL_NUMBER || NextLen <= KINDA_SMALL_NUMBER)
            {
                continue;
            }
            const float CosValue = FVector2D::DotProduct(Prev / PrevLen, Next / NextLen);
            const float AngleDeg = FMath::RadiansToDegrees(
                FMath::Acos(FMath::Clamp(CosValue, -1.0f, 1.0f)));
            if (AngleDeg <= Config.SharpAngleThresholdDeg)
            {
                continue;
            }
            const float Angle01 = SmoothStep01((AngleDeg - Config.SharpAngleThresholdDeg) / Denom);
            const float Locality01 = FMath::Clamp(FMath::Min(PrevLen, NextLen) / FMath::Max(PrevLen, NextLen), 0.0f, 1.0f);
            const float Strength01 = Angle01 * FMath::Lerp(0.6f, 1.0f, Locality01);
            if (Strength01 > 0.02f)
            {
                FCornerMarker& Marker = StableCorners.AddDefaulted_GetRef();
                Marker.Arc = StableInputArcs[i];
                Marker.Strength01 = Strength01;
            }
        }
    }
    int32 WindowSize = FMath::Max(3, Config.SmoothWindowSize);
    if ((WindowSize & 1) == 0)
    {
        ++WindowSize;
    }
    const int32 HalfWindow = WindowSize / 2;
    float BaseSmoothStrength = (StableOutputEndIndex + 1 < 10) ? (Config.SmoothStrength * 0.42f) : (Config.SmoothStrength * 0.72f);
    BaseSmoothStrength = FMath::Clamp(BaseSmoothStrength, 0.0f, 0.90f);
    if (WindowSize < 3 || StableOutputEndIndex < 2 || BaseSmoothStrength <= KINDA_SMALL_NUMBER)
    {
        Output.Last() = LatestObservedPoint;
        return Output;
    }
    const float SafeSigma = FMath::Clamp(Config.SmoothSigma, 0.4f, 3.0f);
    TArray<float> PastKernel;
    PastKernel.SetNumUninitialized(HalfWindow + 1);
    for (int32 Lag = 0; Lag <= HalfWindow; ++Lag)
    {
        const float Dist = static_cast<float>(Lag);
        PastKernel[Lag] = FMath::Exp(-(Dist * Dist) / (2.0f * SafeSigma * SafeSigma));
    }
    const float CornerProtectRadius = 2.25f * ResampleDist;
    const float CornerMinSmoothScale = 0.16f;
    const int32 BoundaryFadeCount = FMath::Clamp(HalfWindow + 2, 2, StableOutputEndIndex + 1);
    const int32 TailOutputCount = Output.Num() - 1 - StableOutputEndIndex;
    const bool bHasTailOutput = (TailOutputCount > 0);
    TArray<FTrackSample> Smoothed = Output;
    Smoothed[0] = Output[0];
    float TerminalArcRadius = ResampleDist;
    if (!bHasTailOutput && StableOutputEndIndex >= 1)
    {
        TerminalArcRadius = FMath::Max(
            ResampleDist,
            DistanceBetweenSamples(Output[StableOutputEndIndex], Output[StableOutputEndIndex - 1]) * 2.0f);
    }
    for (int32 i = 1; i <= StableOutputEndIndex; ++i)
    {
        FVector2D Sum(0.0f, 0.0f);
        float WeightSum = 0.0f;
        const int32 MaxLag = FMath::Min(HalfWindow, i);
        for (int32 Lag = 0; Lag <= MaxLag; ++Lag)
        {
            const int32 Idx = i - Lag;
            const float Weight = PastKernel[Lag];
            Sum += Output[Idx].Position * Weight;
            WeightSum += Weight;
        }
        FVector2D Target = Output[i].Position;
        if (WeightSum > KINDA_SMALL_NUMBER)
        {
            Target = Sum / WeightSum;
        }
        float EdgeFactor = 1.0f;
        if (HalfWindow > 0 && i <= HalfWindow)
        {
            const float X = static_cast<float>(i) / static_cast<float>(HalfWindow + 1);
            EdgeFactor = FMath::Lerp(0.25f, 1.0f, SmoothStep01(X));
        }
        float StableTailBoundaryFactor = 1.0f;
        if (i >= StableOutputEndIndex - BoundaryFadeCount + 1)
        {
            const float X = static_cast<float>(StableOutputEndIndex - i + 1) / static_cast<float>(BoundaryFadeCount);
            const float BoundaryMin = FMath::Lerp(0.28f, 0.55f, TailDirectionConsistency01);
            StableTailBoundaryFactor = FMath::Lerp(BoundaryMin, 1.0f, SmoothStep01(X));
        }
        float TerminalFactor = 1.0f;
        if (!bHasTailOutput)
        {
            const float ArcDistToEnd = StableOutputArcs[StableOutputEndIndex] - StableOutputArcs[i];
            if (ArcDistToEnd < TerminalArcRadius)
            {
                const float X = 1.0f - FMath::Clamp(ArcDistToEnd / TerminalArcRadius, 0.0f, 1.0f);
                const float Anchor01 = SmoothStep01(X);
                TerminalFactor = FMath::Lerp(1.0f, 0.42f, Anchor01);
                Target = FMath::Lerp(Target, LatestObservedPoint.Position, 0.18f * Anchor01);
            }
        }
        float ProtectAccum = 0.0f;
        for (const FCornerMarker& Marker : StableCorners)
        {
            const float ArcDist = FMath::Abs(StableOutputArcs[i] - Marker.Arc);
            if (ArcDist > CornerProtectRadius)
            {
                continue;
            }
            const float W = 1.0f - FMath::Clamp(ArcDist / CornerProtectRadius, 0.0f, 1.0f);
            ProtectAccum += Marker.Strength01 * W;
        }
        const float Protect01 = 1.0f - FMath::Exp(-1.6f * ProtectAccum);
        const float CornerScale = FMath::Lerp(1.0f, CornerMinSmoothScale, Protect01);
        const float LocalStrength = BaseSmoothStrength * EdgeFactor * StableTailBoundaryFactor * TerminalFactor * CornerScale;
        Smoothed[i].Position = FMath::Lerp(Output[i].Position, Target, LocalStrength);
    }
    // 第三步：对尾部输出做轻量平滑，仅修改位置，不修改时间锚点。
    if (bHasTailOutput)
    {
        const float TailBaseStrength = FMath::Clamp(
            BaseSmoothStrength * FMath::Lerp(0.04f, 0.18f, TailDirectionConsistency01),
            0.0f,
            0.20f);
        for (int32 i = StableOutputEndIndex + 1; i < Output.Num() - 1; ++i)
        {
            const FVector2D Prev = Smoothed[i - 1].Position;
            const FVector2D Curr = Output[i].Position;
            FVector2D TailTarget = (Prev * 0.30f) + (Curr * 0.70f);
            if (i >= 2)
            {
                const FVector2D A = Output[i - 1].Position - Output[i - 2].Position;
                const FVector2D B = Output[i].Position - Output[i - 1].Position;
                if (A.SizeSquared() > KINDA_SMALL_NUMBER && B.SizeSquared() > KINDA_SMALL_NUMBER)
                {
                    const float CurrLen = B.Size();
                    const FVector2D Pred = Curr + B.GetSafeNormal() * (0.35f * CurrLen);
                    TailTarget = FMath::Lerp(TailTarget, Pred, 0.22f * TailDirectionConsistency01);
                }
            }
            float LocalConsistency01 = TailDirectionConsistency01;
            float TailCornerScale = 1.0f;
            if (i >= 2)
            {
                const FVector2D A = Output[i - 1].Position - Output[i - 2].Position;
                const FVector2D B = Output[i].Position - Output[i - 1].Position;
                if (A.SizeSquared() > KINDA_SMALL_NUMBER && B.SizeSquared() > KINDA_SMALL_NUMBER)
                {
                    const float Dot = FVector2D::DotProduct(A.GetSafeNormal(), B.GetSafeNormal());
                    const float Dot01 = FMath::Clamp(0.5f * (Dot + 1.0f), 0.0f, 1.0f);
                    LocalConsistency01 = 0.6f * LocalConsistency01 + 0.4f * Dot01;
                    if (Config.SharpAngleThresholdDeg > 0.1f)
                    {
                        const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
                        const float Denom = FMath::Max(1.0f, 180.0f - Config.SharpAngleThresholdDeg);
                        const float TailCorner01 = SmoothStep01((AngleDeg - Config.SharpAngleThresholdDeg) / Denom);
                        TailCornerScale = FMath::Lerp(1.0f, 0.18f, TailCorner01);
                    }
                }
            }
            const int32 DistToLast = (Output.Num() - 1) - i;
            const float TailFade = FMath::Clamp(
                static_cast<float>(DistToLast) / static_cast<float>(FMath::Max(1, TailOutputCount)),
                0.0f,
                1.0f);
            const float LocalStrength = TailBaseStrength * LocalConsistency01 * TailFade * TailCornerScale;
            Smoothed[i].Position = FMath::Lerp(Curr, TailTarget, LocalStrength);
        }
    }
    // 末点强制贴回最新观测点。
    // 这里保留终点覆盖语义：最后一个点代表最新观测位置，
    // 同时携带最新的时间锚点，而不是“平滑后位置的真实采样时刻”。
    Smoothed.Last() = LatestObservedPoint;
    return Smoothed;
}
// 计算轨迹几何特征
// 这里只判断是否存在足够长且足够直的有效线段，不参与任何时间相关逻辑。
// 虽然输入类型为 FTrackSample，但这里只使用 Position，禁止在此处混入 TimeSeconds 判定。
FTrackResult FTrackPreprocessUtils::CalculateTrajectoryFeatures(
    const TArray<FTrackSample>& Samples,
    const FTrackDetectConfig& Config)
{
    FTrackResult Result;
    if (Samples.Num() < 2)
    {
        return Result;
    }
    //最小有效段长度
    const float MinValidSegmentLength = FMath::Max(0.0f, Config.MinValidSegmentLength);
	//最大线偏差,即线段外的点到线段的距离不能超过这个值，否则认为线段被严重弯曲，不是有效线段
    constexpr float MaxLineDeviation = 4.0f;
    //最小特征点步长,过滤掉距离上一个特征点小于1.0单位的点
    constexpr float MinFeaturePointStep = 1.0f;
	//投影回溯容忍,防止因为极小的数值噪声就把本来笔直的挥动误判成弯曲，
    constexpr float ProjectionBacktrackTolerance = 2.0f;
	//最小特征点步长平方
    const float MinFeaturePointStepSq = FMath::Square(MinFeaturePointStep);

    TArray<FVector2D> Points;
    Points.Reserve(Samples.Num());
    Points.Add(Samples[0].Position);
    for (int32 i = 1; i < Samples.Num(); ++i)
    {
        if ((Samples[i].Position - Points.Last()).SizeSquared() > MinFeaturePointStepSq)
        {
            Points.Add(Samples[i].Position);
        }
        else
        {
            Points.Last() = Samples[i].Position;
        }
    }
    if (Points.Num() < 2)
    {
        return Result;
    }
    TArray<float> PrefixLength;
    PrefixLength.SetNumUninitialized(Points.Num());
    PrefixLength[0] = 0.0f;
    for (int32 i = 1; i < Points.Num(); ++i)
    {
        PrefixLength[i] = PrefixLength[i - 1] + FVector2D::Distance(Points[i - 1], Points[i]);
    }
    float BestPathLength = 0.0f;
    for (int32 StartIndex = 0; StartIndex < Points.Num() - 1; ++StartIndex)
    {
        const FVector2D Start = Points[StartIndex];
        for (int32 EndIndex = StartIndex + 1; EndIndex < Points.Num(); ++EndIndex)
        {
            const FVector2D End = Points[EndIndex];
            const FVector2D Segment = End - Start;
            const float SegmentLenSq = Segment.SizeSquared();
            if (SegmentLenSq <= KINDA_SMALL_NUMBER)
            {
                continue;
            }
            const float SegmentLen = FMath::Sqrt(SegmentLenSq);
            const float PathLength = PrefixLength[EndIndex] - PrefixLength[StartIndex];
            if (PathLength < MinValidSegmentLength)
            {
                continue;
            }
            const FVector2D Dir = Segment / SegmentLen;
            bool bIsStraight = true;
            float PrevProjection = 0.0f;
            bool bHasPrevProjection = false;
            for (int32 k = StartIndex + 1; k < EndIndex; ++k)
            {
                const FVector2D ToPoint = Points[k] - Start;
                const float Projection = FVector2D::DotProduct(ToPoint, Dir);
                if (Projection < -MaxLineDeviation || Projection > SegmentLen + MaxLineDeviation)
                {
                    bIsStraight = false;
                    break;
                }
                const FVector2D ClosestPoint = Start + Dir * Projection;
                const float Deviation = FVector2D::Distance(Points[k], ClosestPoint);
                if (Deviation > MaxLineDeviation)
                {
                    bIsStraight = false;
                    break;
                }
                if (bHasPrevProjection && Projection + ProjectionBacktrackTolerance < PrevProjection)
                {
                    bIsStraight = false;
                    break;
                }
                PrevProjection = Projection;
                bHasPrevProjection = true;
            }
            if (!bIsStraight)
            {
                continue;
            }
            if (PathLength > BestPathLength)
            {
                BestPathLength = PathLength;
                Result.bValid = true;
                Result.Start = Start;
                Result.End = End;
                Result.PathLength = PathLength;
            }
        }
    }
    return Result;
}
// 根据有效线段的起点和终点计算攻击方向
EAttackDirection FTrackPreprocessUtils::Direction(const FTrackResult& Segment)
{
    if (!Segment.bValid)
    {
        return EAttackDirection::None;
    }
    const FVector2D Delta = Segment.End - Segment.Start;
    if (Delta.SizeSquared() <= KINDA_SMALL_NUMBER)
    {
        return EAttackDirection::None;
    }
    float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (AngleDeg < 0.0f)
    {
        AngleDeg += 360.0f;
    }
    const float AdjustedAngle = FMath::Fmod(AngleDeg + 22.5f, 360.0f);
    const int32 Sector = static_cast<int32>(AdjustedAngle / 45.0f);
    switch (Sector)
    {
    case 0: return EAttackDirection::Left;
    case 1: return EAttackDirection::DownLeft;
    case 2: return EAttackDirection::Down;
    case 3: return EAttackDirection::DownRight;
    case 4: return EAttackDirection::Right;
    case 5: return EAttackDirection::UpRight;
    case 6: return EAttackDirection::Up;
    case 7: return EAttackDirection::UpLeft;
    default: return EAttackDirection::None;
    }
}
// 计算最近一段时间窗口内的挥动速度
// 注意：这里必须使用原始输入 RawSamples，不能使用预处理后的轨迹。
float FTrackPreprocessUtils::CalculateRecentSwingSpeed(const TArray<FTrackSample>& RawSamples, float WindowSeconds)
{
    if (RawSamples.Num() < 2 || WindowSeconds <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }
    const double EndTime = RawSamples.Last().TimeSeconds;
    const double StartTimeLimit = EndTime - WindowSeconds;
    int32 StartIndex = RawSamples.Num() - 1;
    while (StartIndex > 0 && RawSamples[StartIndex - 1].TimeSeconds >= StartTimeLimit)
    {
        --StartIndex;
    }
    float Distance = 0.0f;
    for (int32 i = StartIndex + 1; i < RawSamples.Num(); ++i)
    {
        Distance += DistanceBetweenSamples(RawSamples[i - 1], RawSamples[i]);
    }
    const double Duration = RawSamples.Last().TimeSeconds - RawSamples[StartIndex].TimeSeconds;
    if (Duration <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }
    return Distance / static_cast<float>(Duration);
}
// 将速度归一化到 0~1 分数区间
float FTrackPreprocessUtils::NormalizeSpeedScore(float Speed, float MinSwingSpeed)
{
    if (MinSwingSpeed <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }
    const float Ratio = Speed / MinSwingSpeed;
    if (Ratio <= 1.0f)
    {
        return FMath::Clamp(Ratio * 0.35f, 0.0f, 0.35f);
    }
    return FMath::Clamp(0.35f + (Ratio - 1.0f) * 0.65f, 0.35f, 1.0f);
}
// 在两个带时间戳的采样点之间做同步插值
FTrackSample FTrackPreprocessUtils::LerpSample(const FTrackSample& A, const FTrackSample& B, float Alpha)
{
    FTrackSample Out;
    Out.Position = FMath::Lerp(A.Position, B.Position, Alpha);
    Out.TimeSeconds = FMath::Lerp(A.TimeSeconds, B.TimeSeconds, Alpha);
    return Out;
}
// 计算两个采样点之间的位置距离
float FTrackPreprocessUtils::DistanceBetweenSamples(const FTrackSample& A, const FTrackSample& B)
{
    return FVector2D::Distance(A.Position, B.Position);
}

