#include "Components/CombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Character/Base.h"
#include "Engine/World.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    CombatStatusSwitch.Initialize();
    CombatDamage.Initialize();
    CombatSampling.ClearPendingAttack();
    CombatSampling.ClearSamplingBuffer();

    CombatSampling.MinSampleDistance = MinSampleDistance;
    CombatStatusSwitch.DirectionSwitchCooldown = DirectionSwitchCooldown;
    CombatStatusSwitch.AttackRequestCooldown = AttackRequestCooldown;
    CombatStatusSwitch.CounterAttackValidWindow = CounterAttackValidWindow;

    SetComponentTickEnabled(false);
}

void UCombatComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWorld* World = GetWorld();
    if (!World || !GetOwner())
    {
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    CombatStatusSwitch.RefreshAttackState(CurrentTime);

    if (CombatSampling.HasPendingAttack() && !CombatStatusSwitch.IsLockedState())
    {
        const EAttackDirection PendingDirection = CombatSampling.GetPendingDirection();
        const float PendingTrackScore = CombatSampling.GetPendingTrackScore();
        CombatSampling.ClearPendingAttack();
        PerformAttackAndBind(PendingDirection, PendingTrackScore);
    }

    const bool bAttackActive = CombatStatusSwitch.HasActiveAttack(CurrentTime);
    if (!bAttackActive && CombatDamage.IsWeaponTraceWindowOpen())
    {
        CombatDamage.ForceDisableWeaponTrace(GetOwner());
    }

    if (!bAttackActive && !CombatSampling.HasPendingAttack())
    {
        CombatStatusSwitch.CurrentDirection = EAttackDirection::None;
    }

    UpdateTickEnabled();
}

void UCombatComponent::PerformAttackAndBind(EAttackDirection Direction, float TrackScore)
{
    UAnimMontage* PlayedMontage = CombatDamage.PerformAttack(
        GetOwner(),
        GetWorld(),
        AttackMoveDataAsset,
        CombatStatusSwitch,
        CombatSampling,
        Direction,
        TrackScore
    );

    UpdateTickEnabled();

    if (!PlayedMontage)
    {
        return;
    }

    if (!BindAttackMontageDelegates(PlayedMontage))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[攻击交互][委托绑定失败] 角色=%s Montage=%s，立即取消本次攻击状态"),
            *GetNameSafe(GetOwner()), *GetNameSafe(PlayedMontage));

        FAttackAnimationPlayer::StopAttackMontage(GetOwner(), PlayedMontage, 0.10f);
        FinalizeCurrentAttack(nullptr, true, TEXT("DelegateBindFailed"));
    }
}

bool UCombatComponent::BindAttackMontageDelegates(UAnimMontage* Montage)
{
    if (!Montage)
    {
        return false;
    }

    UAnimInstance* AnimInstance = FAttackAnimationPlayer::ResolveAnimInstance(GetOwner());
    if (!AnimInstance)
    {
        return false;
    }

    CurrentAttackMontage = Montage;

    FOnMontageBlendingOutStarted BlendingOutDelegate;
    BlendingOutDelegate.BindUObject(this, &UCombatComponent::HandleAttackMontageBlendingOut);
    AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UCombatComponent::HandleAttackMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

    return true;
}

void UCombatComponent::HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    FinalizeCurrentAttack(Montage, bInterrupted, TEXT("BlendingOut"));
}

void UCombatComponent::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // Blend Out 通常先触发；Ended 作为兜底。相同蒙太奇只会清理一次。
    FinalizeCurrentAttack(Montage, bInterrupted, TEXT("Ended"));
}

void UCombatComponent::FinalizeCurrentAttack(
    UAnimMontage* Montage,
    bool bInterrupted,
    const TCHAR* EventSource
)
{
    if (Montage && CurrentAttackMontage != Montage)
    {
        return;
    }

    if (!Montage && !CurrentAttackMontage && !CombatStatusSwitch.bAttackMontageActive && !CombatStatusSwitch.bConvertedGuardActive)
    {
        return;
    }

    UAnimMontage* FinishedMontage = Montage ? Montage : CurrentAttackMontage.Get();
    CurrentAttackMontage = nullptr;

    CombatDamage.ForceDisableWeaponTrace(GetOwner());
    CombatStatusSwitch.CompleteCurrentAttack(GetOwner(), CombatSampling, bInterrupted);

    if (ABase* OwnerCharacter = Cast<ABase>(GetOwner()))
    {
        OwnerCharacter->NotifyWeaponAttackFinished(bInterrupted);
    }

    if (bInterrupted)
    {
        CombatDamage.ResetAfterInterrupt();
    }
    else
    {
        CombatDamage.ResetAfterAttack();
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][蒙太奇生命周期结束] 角色=%s Montage=%s 来源=%s 是否中断=%s"),
        *GetNameSafe(GetOwner()),
        *GetNameSafe(FinishedMontage),
        EventSource,
        bInterrupted ? TEXT("是") : TEXT("否"));

    UpdateTickEnabled();
}

void UCombatComponent::BeginAttackSampling(float CurrentTime)
{
    CombatSampling.MinSampleDistance = MinSampleDistance;
    CombatSampling.BeginAttackSampling(GetOwner(), CombatStatusSwitch, CurrentTime);
    UpdateTickEnabled();
}

void UCombatComponent::EndAttackSampling()
{
    CombatSampling.EndAttackSampling(GetWorld(), CombatStatusSwitch);
    UpdateTickEnabled();
}

void UCombatComponent::CacheMouseInput(const FVector2D& Input, float CurrentTime)
{
    EAttackDirection Direction = EAttackDirection::None;
    float TrackScore = 0.0f;

    const bool bHasAttackRequest = CombatSampling.CacheMouseInput(
        Input,
        CurrentTime,
        CombatStatusSwitch,
        Direction,
        TrackScore
    );

    if (bHasAttackRequest)
    {
        PerformAttackAndBind(Direction, TrackScore);
    }

    UpdateTickEnabled();
}

void UCombatComponent::StartBlock()
{
    CombatStatusSwitch.StartBlock(GetOwner());
}

void UCombatComponent::StopBlock()
{
    CombatStatusSwitch.StopBlock(GetOwner());
}

bool UCombatComponent::IsBlocking() const
{
    return CombatStatusSwitch.IsBlocking();
}

EAttackState UCombatComponent::GetAttackState() const
{
    return CombatStatusSwitch.GetAttackState();
}

EAttackDirection UCombatComponent::GetCurrentDirection() const
{
    return CombatStatusSwitch.GetCurrentDirection();
}

int32 UCombatComponent::GetAttackTriggerCounter() const
{
    return CombatStatusSwitch.GetAttackTriggerCounter();
}

bool UCombatComponent::IsAttackActive() const
{
    return CombatStatusSwitch.IsAttackActive(GetWorld());
}

float UCombatComponent::GetCurrentDamageModifier() const
{
    return CombatDamage.GetCurrentDamageModifier();
}

float UCombatComponent::GetCurrentAttackDamage() const
{
    return CombatDamage.GetCurrentAttackDamage();
}

float UCombatComponent::GetCurrentBaseDamage() const
{
    return CombatDamage.GetCurrentBaseDamage();
}

FName UCombatComponent::GetCurrentAttackType() const
{
    return CombatDamage.GetCurrentAttackType();
}

void UCombatComponent::EnableWeaponTrace()
{
    CombatDamage.EnableWeaponTrace(GetOwner());
}

void UCombatComponent::DisableWeaponTrace()
{
    CombatDamage.DisableWeaponTrace(GetOwner());
}

void UCombatComponent::InterruptCurrentAttack()
{
    UAnimMontage* MontageToStop = CurrentAttackMontage.Get();

    if (MontageToStop)
    {
        FAttackAnimationPlayer::StopAttackMontage(GetOwner(), MontageToStop, 0.10f);
    }

    // Montage_Stop 通常同步触发 Blend Out；若未触发，则这里兜底清理。
    if (CurrentAttackMontage == MontageToStop || (!MontageToStop && CombatStatusSwitch.HasActiveAttack(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f)))
    {
        FinalizeCurrentAttack(MontageToStop, true, TEXT("InterruptFallback"));
    }

    UpdateTickEnabled();
}

void UCombatComponent::UpdateTickEnabled()
{
    const UWorld* World = GetWorld();
    const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

    const bool bNeedsTick =
        CombatStatusSwitch.HasActiveAttack(CurrentTime) ||
        CombatSampling.HasPendingAttack() ||
        CombatDamage.IsWeaponTraceWindowOpen() ||
        CurrentAttackMontage != nullptr;

    SetComponentTickEnabled(bNeedsTick);
}

void UCombatComponent::StartDeflect()
{
    InterruptCurrentAttack();
    CombatStatusSwitch.StartDeflect(GetOwner());
    CombatDamage.ResetForDeflect();
}

void UCombatComponent::EndDeflect()
{
    CombatStatusSwitch.EndDeflect(GetOwner());
}

bool UCombatComponent::IsDeflecting() const
{
    return CombatStatusSwitch.IsDeflecting();
}
