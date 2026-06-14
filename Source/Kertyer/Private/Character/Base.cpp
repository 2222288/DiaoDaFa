#include "Character/Base.h"
#include "Weapon/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CombatComponent.h"
#include "AnimationLogic/AttackAnimationPlayer.h"
#include "DataAsset/CombatReactionAnimationData.h"
#include "Character/Hostile.h"
#include "Components/LockOn.h"
#include "EngineUtils.h"
#include "Engine/World.h"

namespace
{
    const TCHAR* BaseAttackDirectionToChinese(EAttackDirection Direction)
    {
        switch (Direction)
        {
        case EAttackDirection::None: return TEXT("无");
        case EAttackDirection::Up: return TEXT("上");
        case EAttackDirection::UpRight: return TEXT("右上");
        case EAttackDirection::Right: return TEXT("右");
        case EAttackDirection::DownRight: return TEXT("右下");
        case EAttackDirection::Down: return TEXT("下");
        case EAttackDirection::DownLeft: return TEXT("左下");
        case EAttackDirection::Left: return TEXT("左");
        case EAttackDirection::UpLeft: return TEXT("左上");
        default: return TEXT("未知方向");
        }
    }

    FString BaseSafeActorName(const AActor* Actor)
    {
        return IsValid(Actor) ? Actor->GetName() : TEXT("无");
    }

    FString BaseSafeName(FName Name)
    {
        return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
    }

    int32 BaseDirectionToIndex(EAttackDirection Direction)
    {
        switch (Direction)
        {
        case EAttackDirection::Up: return 0;
        case EAttackDirection::UpRight: return 1;
        case EAttackDirection::Right: return 2;
        case EAttackDirection::DownRight: return 3;
        case EAttackDirection::Down: return 4;
        case EAttackDirection::DownLeft: return 5;
        case EAttackDirection::Left: return 6;
        case EAttackDirection::UpLeft: return 7;
        default: return INDEX_NONE;
        }
    }

    int32 BaseCircularDirectionDelta(EAttackDirection A, EAttackDirection B)
    {
        const int32 IndexA = BaseDirectionToIndex(A);
        const int32 IndexB = BaseDirectionToIndex(B);

        if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
        {
            return INDEX_NONE;
        }

        const int32 RawDelta = FMath::Abs(IndexA - IndexB);
        return FMath::Min(RawDelta, 8 - RawDelta);
    }

    EWeaponContactDirectionRelation BaseResolveDirectionRelation(
        EAttackDirection GuardDirection,
        EAttackDirection IncomingDirection
    )
    {
        const int32 Delta = BaseCircularDirectionDelta(GuardDirection, IncomingDirection);

        if (Delta == 4)
        {
            return EWeaponContactDirectionRelation::Opposite;
        }

        if (Delta == 3)
        {
            return EWeaponContactDirectionRelation::NearOpposite;
        }

        if (Delta != INDEX_NONE)
        {
            return EWeaponContactDirectionRelation::NonOpposite;
        }

        return EWeaponContactDirectionRelation::Invalid;
    }

    bool BaseIsWeaponInActiveAttackState(EWeaponState State)
    {
        return State == EWeaponState::Attacking
            || State == EWeaponState::ContactWindowOpen;
    }

    const TCHAR* BaseDirectionRelationToChinese(EWeaponContactDirectionRelation Relation)
    {
        switch (Relation)
        {
        case EWeaponContactDirectionRelation::Opposite: return TEXT("对向格挡");
        case EWeaponContactDirectionRelation::NearOpposite: return TEXT("偏对向格挡");
        case EWeaponContactDirectionRelation::NonOpposite: return TEXT("错误方向格挡");
        case EWeaponContactDirectionRelation::Invalid: return TEXT("无效方向格挡");
        default: return TEXT("未知格挡");
        }
    }
}

ABase::ABase()
{
    PrimaryActorTick.bCanEverTick = true;

    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}

void ABase::BeginPlay()
{
    Super::BeginPlay();

    if (!CurrentWeapon && DefaultWeaponClass && GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = this;

        AWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AWeaponBase>(DefaultWeaponClass, GetActorTransform(), SpawnParams);
        SetCurrentWeapon(SpawnedWeapon);
    }
    else if (CurrentWeapon)
    {
        SetCurrentWeapon(CurrentWeapon);
    }
}

void ABase::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
    CurrentWeapon = NewWeapon;

    if (!CurrentWeapon)
    {
        return;
    }

    CurrentWeapon->SetCurrentHolder(this);
    CurrentWeapon->AttachToComponent(
        GetMesh(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        WeaponAttachSocketName
    );
}

void ABase::NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier, float CounterAttackValidWindow)
{
    if (!CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[攻击交互][武器数据下发失败] 角色=%s 原因=CurrentWeapon为空"),
            *GetName());
        return;
    }

    FWeaponAttackData AttackData;
    AttackData.AttackDirection = AttackDirection;
    AttackData.AttackStartTime = AttackStartTime;
    AttackData.AttackType = AttackType;
    AttackData.BaseDamage = BaseDamage;
    AttackData.DamageModifier = DamageModifier;
    AttackData.CounterAttackValidWindow = CounterAttackValidWindow > 0.0f ? CounterAttackValidWindow : 0.5f;

    CurrentWeapon->ReceiveAttackData(AttackData);

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][武器数据下发] 角色=%s 武器=%s 攻击ID=%s 开始时间=%.3f 基础伤害=%.2f 倍率=%.3f 响应窗口=%.3f"),
        *GetName(),
        *CurrentWeapon->GetName(),
        *AttackType.ToString(),
        AttackStartTime,
        BaseDamage,
        DamageModifier,
        AttackData.CounterAttackValidWindow);
}
void ABase::Treat(float Treatmentamount)
{
    if (Treatmentamount <= 0.0f || CurrentHealth >= MaxHealth)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + Treatmentamount, 0.0f, MaxHealth);
}

float ABase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (CurrentHealth <= 0.0f || DamageAmount <= 0.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[攻击交互][扣血忽略] 受击者=%s 当前血量=%.2f 输入伤害=%.2f 伤害来源=%s 控制器=%s"),
            *GetName(),
            CurrentHealth,
            DamageAmount,
            *BaseSafeActorName(DamageCauser),
            EventInstigator ? *EventInstigator->GetName() : TEXT("无"));
        return 0.0f;
    }

    const float OldHealth = CurrentHealth;
    const float ActualDamage = FMath::Min(CurrentHealth, DamageAmount);

    Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

    const bool bShouldSuppressHitReaction = bSuppressNextNonLethalHitReaction;
    bSuppressNextNonLethalHitReaction = false;

    if (CurrentHealth > 0.0f)
    {
        if (!bShouldSuppressHitReaction)
        {
            PlayHitReaction();
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[攻击交互][扣血但跳过Hit反应] 受击者=%s 实际扣血=%.2f 原因=错误方向格挡仍播放Guard"),
                *GetName(),
                ActualDamage);
        }
    }
    else
    {
        PlayCombatReaction(
            ECombatReactionType::Death,
            EWeaponContactResult::Hit,
            EAttackDirection::None,
            false,
            false
        );
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][扣血结算] 受击者=%s 伤害来源=%s 控制器=%s 输入伤害=%.2f 实际扣血=%.2f 血量=%.2f -> %.2f / %.2f"),
        *GetName(),
        *BaseSafeActorName(DamageCauser),
        EventInstigator ? *EventInstigator->GetName() : TEXT("无"),
        DamageAmount,
        ActualDamage,
        OldHealth,
        CurrentHealth,
        MaxHealth);

    if (CurrentHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[攻击交互][角色死亡] 死亡角色=%s 最后一击来源=%s 最后一击伤害=%.2f"),
            *GetName(),
            *BaseSafeActorName(DamageCauser),
            ActualDamage);
    }

    return ActualDamage;
}
void ABase::EnableWeaponTrace()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->EnableWeaponTrace();
    }
}

void ABase::DisableWeaponTrace()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->DisableWeaponTrace();
    }
}

void ABase::InterruptCurrentAttackByBodyHit(AActor* DamageCauser)
{
    if (CurrentWeapon)
    {
        CurrentWeapon->ForceStopWeaponInteraction(TEXT("身体被武器命中，本次攻击被打断"));
    }

    if (UCombatComponent* CombatComponent = FindComponentByClass<UCombatComponent>())
    {
        CombatComponent->InterruptCurrentAttack();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][角色攻击被打断] 角色=%s 打断来源=%s"),
        *GetName(),
        DamageCauser ? *DamageCauser->GetName() : TEXT("无"));
}

bool ABase::PlayCombatReaction(
    ECombatReactionType ReactionType,
    EWeaponContactResult ContactResult,
    EAttackDirection Direction,
    bool bSelfIsSlower,
    bool bValidTimedResponse
)
{
    float IgnoredPlayedLength = 0.0f;

    return PlayCombatReactionAndGetLength(
        ReactionType,
        ContactResult,
        Direction,
        bSelfIsSlower,
        bValidTimedResponse,
        IgnoredPlayedLength
    );
}

bool ABase::PlayCombatReactionAndGetLength(
    ECombatReactionType ReactionType,
    EWeaponContactResult ContactResult,
    EAttackDirection Direction,
    bool bSelfIsSlower,
    bool bValidTimedResponse,
    float& OutPlayedLength
)
{
    OutPlayedLength = 0.0f;

    if (!CombatReactionAnimationData)
    {
        return false;
    }

    const FCombatReactionAnimation* ReactionRow =
        CombatReactionAnimationData->FindBestReaction(
            ReactionType,
            ContactResult,
            Direction,
            bSelfIsSlower,
            bValidTimedResponse
        );

    if (!ReactionRow)
    {
        return false;
    }

    const FAttackAnimationPlayResult Result =
        FAttackAnimationPlayer::PlayReactionMontage(this, *ReactionRow);

    if (!Result.bSucceeded)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[战斗反应动画][播放失败] 角色=%s 类型=%d 原因=%s Montage=%s Section=%s"),
            *GetName(),
            static_cast<int32>(ReactionType),
            *Result.ErrorMessage,
            *GetNameSafe(ReactionRow->Montage),
            *ReactionRow->MontageSection.ToString()
        );

        return false;
    }

    OutPlayedLength = Result.PlayedLength;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[战斗反应动画][播放成功] 角色=%s 类型=%d Montage=%s Section=%s 时长=%.3f"),
        *GetName(),
        static_cast<int32>(ReactionType),
        *GetNameSafe(Result.PlayedMontage),
        *Result.PlayedSection.ToString(),
        Result.PlayedLength
    );

    return true;
}

void ABase::PlayWeaponContactReaction(
    const FWeaponContactResolveOutput& ResolveOutput,
    EWeaponContactSide SelfSide
)
{
    if (SelfSide == EWeaponContactSide::None)
    {
        return;
    }

    const bool bSelfIsSlower = ResolveOutput.SlowerSide == SelfSide;

    const bool bSelfIsDamaged =
        ResolveOutput.DamagedSide == SelfSide ||
        ResolveOutput.DamagedSide == EWeaponContactSide::Both;

    ECombatReactionType ReactionType = ECombatReactionType::None;

    switch (ResolveOutput.Result)
    {
    case EWeaponContactResult::Clash:
        // 非等速，方向能架住：
        // 较慢方播放格挡；
        // 较快方继续保持攻击动画，不强行切 Clash。
        if (bSelfIsSlower && ResolveOutput.bIsValidTimedResponse)
        {
            ReactionType = ECombatReactionType::Guard;
        }
        else
        {
            return;
        }
        break;

    case EWeaponContactResult::Deflect:
        ReactionType = ECombatReactionType::Deflect;
        StartDeflect();
        break;

    case EWeaponContactResult::Interrupt:
        if (!bSelfIsDamaged)
        {
            return;
        }

        ReactionType = ECombatReactionType::Interrupt;
        break;

    case EWeaponContactResult::Hit:
        // 只有受伤方播放受击反应。
        // 优势方继续保持攻击动画。
        if (!bSelfIsDamaged)
        {
            return;
        }

        ReactionType = ECombatReactionType::Hit;
        break;

    case EWeaponContactResult::Ignore:
    default:
        return;
    }

    EAttackDirection ReactionDirection = EAttackDirection::None;

    if (CurrentWeapon)
    {
        ReactionDirection = CurrentWeapon->GetCurrentAttackDirection();
    }

    PlayCombatReaction(
        ReactionType,
        ResolveOutput.Result,
        ReactionDirection,
        bSelfIsSlower,
        ResolveOutput.bIsValidTimedResponse
    );
}

void ABase::PlayHitReaction()
{
    PlayCombatReaction(
        ECombatReactionType::Hit,
        EWeaponContactResult::Hit,
        EAttackDirection::None,
        false,
        false
    );
}

void ABase::StartDeflect()
{
    if (bIsDeflecting)
    {
        return;
    }

    bIsDeflecting = true;

    if (CurrentWeapon)
    {
        CurrentWeapon->ForceStopWeaponInteraction(TEXT("进入弹刀状态，停止当前武器交互"));
    }

    // 如果这个角色有 CombatComponent，就同步打断。
    // 玩家有，敌人没有也没关系。
    if (UCombatComponent* FoundCombatComponent = FindComponentByClass<UCombatComponent>())
    {
        FoundCombatComponent->InterruptCurrentAttack();
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[攻击交互][弹刀开始] 角色=%s"),
        *GetName()
    );
}

void ABase::EndDeflect()
{
    if (!bIsDeflecting)
    {
        return;
    }

    bIsDeflecting = false;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[攻击交互][弹刀结束] 角色=%s"),
        *GetName()
    );
}

ABase* ABase::FindPreAttackGuardOpponent() const
{
    if (const ULockOn* LockOn = FindComponentByClass<ULockOn>())
    {
        if (ABase* LockedTarget = Cast<ABase>(LockOn->GetLockTarget()))
        {
            return LockedTarget;
        }
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    ABase* BestTarget = nullptr;
    float BestDistSq = FMath::Square(PreAttackGuardSearchRadius);

    for (TActorIterator<ABase> It(World); It; ++It)
    {
        ABase* Candidate = *It;

        if (!Candidate || Candidate == this || Candidate->CurrentHealth <= 0.0f)
        {
            continue;
        }

        const AWeaponBase* CandidateWeapon = Candidate->GetCurrentWeapon();
        if (!CandidateWeapon)
        {
            continue;
        }

        const FWeaponAttackData& CandidateAttackData = CandidateWeapon->GetCurrentAttackData();
        if (!CandidateAttackData.IsValid())
        {
            continue;
        }

        if (!BaseIsWeaponInActiveAttackState(CandidateWeapon->GetWeaponState()))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

bool ABase::TryConvertAttackToGuard(
    EAttackDirection GuardDirection,
    float GuardRequestTime,
    float& OutGuardDuration)
{
    OutGuardDuration = 0.0f;

    if (GuardDirection == EAttackDirection::None)
    {
        return false;
    }

    ABase* Opponent = FindPreAttackGuardOpponent();
    if (!Opponent || Opponent == this)
    {
        return false;
    }

    AWeaponBase* OpponentWeapon = Opponent->GetCurrentWeapon();
    if (!OpponentWeapon)
    {
        return false;
    }

    const FWeaponAttackData& OpponentAttackData = OpponentWeapon->GetCurrentAttackData();

    if (!OpponentAttackData.IsValid())
    {
        return false;
    }

    if (!BaseIsWeaponInActiveAttackState(OpponentWeapon->GetWeaponState()))
    {
        return false;
    }

    const float OpponentAttackStartTime = OpponentAttackData.AttackStartTime;
    const float TimeDelta = GuardRequestTime - OpponentAttackStartTime;

    // 必须是“我慢于对方”。等速不在这里转 Guard，仍留给原来的 Deflect 逻辑。
    if (TimeDelta <= EqualAttackTimeTolerance)
    {
        return false;
    }

    const float ValidWindow = OpponentAttackData.CounterAttackValidWindow > 0.0f
        ? OpponentAttackData.CounterAttackValidWindow
        : 0.5f;

    if (TimeDelta > ValidWindow)
    {
        return false;
    }

    const EWeaponContactDirectionRelation DirectionRelation =
        BaseResolveDirectionRelation(GuardDirection, OpponentAttackData.AttackDirection);

    const bool bPlayedGuard = PlayCombatReactionAndGetLength(
        ECombatReactionType::Guard,
        EWeaponContactResult::Clash,
        GuardDirection,
        true,
        true,
        OutGuardDuration
    );

    if (!bPlayedGuard)
    {
        return false;
    }

    const float IncomingDamage = OpponentWeapon->GetCurrentAttackDamage();

    switch (DirectionRelation)
    {
    case EWeaponContactDirectionRelation::Opposite:
        // 对向：抵消伤害，并让格挡方下一次真正攻击更快。
        GrantNextAttackSpeedBonus(PerfectGuardNextAttackSpeedMultiplier);

        Opponent->CancelCurrentAttackByGuard(
            this,
            TEXT("对向格挡成功：抵消此次攻击，并给予格挡方下一次攻击速度加成")
        );
        break;

    case EWeaponContactDirectionRelation::NearOpposite:
        // 偏对向：抵消伤害，无加成。
        Opponent->CancelCurrentAttackByGuard(
            this,
            TEXT("偏对向格挡成功：抵消此次攻击，无速度加成")
        );
        break;

    case EWeaponContactDirectionRelation::NonOpposite:
    case EWeaponContactDirectionRelation::Invalid:
    default:
        // 错误方向：仍播放 Guard，但吃满伤害；非致命时不切 Hit。
        ApplyDamageWithoutNonLethalHitReaction(
            IncomingDamage,
            Opponent->GetController(),
            OpponentWeapon
        );

        Opponent->CancelCurrentAttackByGuard(
            this,
            TEXT("错误方向格挡：播放Guard，但承受完整伤害")
        );
        break;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][攻击前转格挡] 格挡者=%s 对手=%s 格挡方向=%s 对手方向=%s 关系=%s 时间差=%.3f 有效窗口=%.3f 伤害=%.2f Guard时长=%.3f"),
        *GetName(),
        *Opponent->GetName(),
        BaseAttackDirectionToChinese(GuardDirection),
        BaseAttackDirectionToChinese(OpponentAttackData.AttackDirection),
        BaseDirectionRelationToChinese(DirectionRelation),
        TimeDelta,
        ValidWindow,
        IncomingDamage,
        OutGuardDuration);

    return true;
}

void ABase::GrantNextAttackSpeedBonus(float PlayRateMultiplier)
{
    if (PlayRateMultiplier <= 1.0f)
    {
        return;
    }

    NextAttackPlayRateModifier = FMath::Max(NextAttackPlayRateModifier, PlayRateMultiplier);

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][下一次攻击加速] 角色=%s 下次攻击倍率=%.3f"),
        *GetName(),
        NextAttackPlayRateModifier);
}

float ABase::ConsumeNextAttackPlayRateModifier()
{
    const float Result = FMath::Max(0.1f, NextAttackPlayRateModifier);
    NextAttackPlayRateModifier = 1.0f;
    return Result;
}

void ABase::CancelCurrentAttackByGuard(AActor* GuardActor, const FString& Reason)
{
    if (CurrentWeapon)
    {
        CurrentWeapon->ForceStopWeaponInteraction(Reason);
    }

    FAttackAnimationPlayer::StopAttackMontage(this, nullptr, 0.10f);

    if (UCombatComponent* CombatComponent = FindComponentByClass<UCombatComponent>())
    {
        CombatComponent->InterruptCurrentAttack();
    }

    OnAttackCancelledByGuard(GuardActor, Reason);

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][攻击被格挡取消] 被取消者=%s 格挡者=%s 原因=%s"),
        *GetName(),
        *BaseSafeActorName(GuardActor),
        *Reason);
}

void ABase::ApplyDamageWithoutNonLethalHitReaction(
    float DamageAmount,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    if (DamageAmount <= 0.0f)
    {
        return;
    }

    bSuppressNextNonLethalHitReaction = true;

    UGameplayStatics::ApplyDamage(
        this,
        DamageAmount,
        EventInstigator,
        DamageCauser,
        UDamageType::StaticClass()
    );

    // 如果 TakeDamage 因非法伤害等原因没有消耗该标记，这里兜底清掉。
    bSuppressNextNonLethalHitReaction = false;
}

void ABase::OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason)
{
    (void)GuardActor;
    (void)Reason;
}