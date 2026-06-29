#include "Character/Hostile.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Blueprint/UserWidget.h"
#include "Components/BaseHealthComponent.h"
#include "Components/WidgetComponent.h"
#include "DataAsset/AttackMoveDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Widget/EnemyHealthBarWidget.h"

AHostile::AHostile()
{
    PrimaryActorTick.bCanEverTick = false;

    HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));

    // 构造阶段只声明父组件；Socket 和偏移在 BeginPlay 中读取可配置属性后应用。
    HealthBarWidget->SetupAttachment(GetMesh());
    HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
    HealthBarWidget->SetDrawSize(FVector2D(200.0f, 24.0f));
    HealthBarWidget->SetVisibility(true);
    HealthBarWidget->SetHiddenInGame(false);
    HealthBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bUseControllerRotationYaw = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bOrientRotationToMovement = false;
        GetCharacterMovement()->bUseControllerDesiredRotation = true;
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    }
}

void AHostile::BeginPlay()
{
    Super::BeginPlay();

    if (HealthBarWidget && GetMesh())
    {
        HealthBarWidget->AttachToComponent(
            GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            HealthBarAttachSocketName
        );

        HealthBarWidget->SetRelativeLocation(HealthBarOffset);
        HealthBarWidget->InitWidget();
    }

    if (BaseHealthComponent)
    {
        BaseHealthComponent->OnHealthChanged.AddUniqueDynamic(this, &AHostile::HandleHealthChanged);
    }

    UpdateHealthBar(GetHealthPercent());
}

void AHostile::HandleHealthChanged(float OldHealth, float NewHealth, float InMaxHealth)
{
    static_cast<void>(OldHealth);

    UpdateHealthBar(InMaxHealth > 0.0f ? NewHealth / InMaxHealth : 0.0f);
}

void AHostile::UpdateHealthBar(float HealthPercent)
{
    if (!HealthBarWidget)
    {
        return;
    }

    UUserWidget* RawWidget = HealthBarWidget->GetUserWidgetObject();

    if (!RawWidget)
    {
        HealthBarWidget->InitWidget();
        RawWidget = HealthBarWidget->GetUserWidgetObject();
    }

    if (UEnemyHealthBarWidget* Widget = Cast<UEnemyHealthBarWidget>(RawWidget))
    {
        Widget->SetHealthPercent(HealthPercent);
    }
}

bool AHostile::CanAttack() const
{
    return bCanAttack && !bIsAttacking;
}

float AHostile::GetCurrentAttackDamage() const
{
    return CurrentAttackDamage;
}

const FAttackMoveData* AHostile::GetRandomAttackData() const
{
    return AttackMoveDataAsset ? AttackMoveDataAsset->GetRandomAttack() : nullptr;
}

void AHostile::Attack()
{
    if (!CanAttack())
    {
        return;
    }

    const FAttackMoveData* AttackData = GetRandomAttackData();

    if (!AttackData || !AttackData->AttackMontage)
    {
        return;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    float GuardDuration = 0.0f;

    if (TryConvertAttackToGuard(AttackData->AttackDirection, CurrentTime, GuardDuration))
    {
        bIsAttacking = true;
        bCanAttack = false;
        bWeaponAttackCycleActive = false;

        CurrentAttackDamage = 0.0f;
        CurrentHostileAttackDirection = AttackData->AttackDirection;
        CurrentHostileAttackType = TEXT("Guard");
        CurrentHostileAttackStartTime = CurrentTime;

        const float SafeGuardDuration = FMath::Max(0.1f, GuardDuration);

        GetWorldTimerManager().ClearTimer(ConvertedGuardFinishTimer);
        GetWorldTimerManager().SetTimer(
            ConvertedGuardFinishTimer,
            this,
            &AHostile::FinishConvertedGuard,
            SafeGuardDuration,
            false
        );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("敌人攻击请求被转换为格挡: Direction=%d, Duration=%f"),
            static_cast<int32>(CurrentHostileAttackDirection),
            SafeGuardDuration
        );

        return;
    }

    const float AttackPlayRate = ConsumeNextAttackPlayRateModifier();

    const float ActualCounterAttackValidWindow =
        AttackData->CounterAttackValidWindow > 0.0f
        ? AttackData->CounterAttackValidWindow
        : 0.5f;

    CurrentAttackDamage = AttackData->Damage;
    CurrentHostileAttackDirection = AttackData->AttackDirection;
    CurrentHostileAttackType = AttackData->AttackID;
    CurrentHostileAttackStartTime = CurrentTime;

    const FAttackAnimationPlayResult AnimationResult =
        FAttackAnimationPlayer::PlayAttackMontage(this, *AttackData, AttackPlayRate);

    if (!AnimationResult.bSucceeded)
    {
        if (AttackPlayRate > 1.0f)
        {
            GrantNextAttackSpeedBonus(AttackPlayRate);
        }

        CompleteAttackState(nullptr, true, TEXT("PlayFailed"));

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("敌人攻击动画播放失败: AttackID=%s, Section=%s, PlayRate=%f, Error=%s"),
            *AttackData->AttackID.ToString(),
            *AttackData->MontageSection.ToString(),
            AttackPlayRate,
            *AnimationResult.ErrorMessage
        );

        return;
    }

    bIsAttacking = true;
    bCanAttack = false;

    if (!BindAttackMontageDelegates(AnimationResult.PlayedMontage))
    {
        FAttackAnimationPlayer::StopAttackMontage(this, AnimationResult.PlayedMontage, 0.10f);
        CompleteAttackState(nullptr, true, TEXT("DelegateBindFailed"));
        return;
    }

    NotifyWeaponAttackStarted(
        CurrentHostileAttackDirection,
        CurrentHostileAttackType,
        CurrentHostileAttackStartTime,
        AttackData->Damage,
        1.0f,
        ActualCounterAttackValidWindow
    );

    bWeaponAttackCycleActive = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("敌人发起攻击: AttackID=%s, Section=%s, Damage=%f, ReportedDuration=%f, ResponseWindow=%f, PlayRate=%f"),
        *CurrentHostileAttackType.ToString(),
        *AnimationResult.PlayedSection.ToString(),
        CurrentAttackDamage,
        AnimationResult.PlayedLength,
        ActualCounterAttackValidWindow,
        AttackPlayRate
    );
}

bool AHostile::BindAttackMontageDelegates(UAnimMontage* Montage)
{
    if (!Montage)
    {
        return false;
    }

    UAnimInstance* AnimInstance = FAttackAnimationPlayer::ResolveAnimInstance(this);

    if (!AnimInstance)
    {
        return false;
    }

    CurrentAttackMontage = Montage;

    FOnMontageBlendingOutStarted BlendingOutDelegate;
    BlendingOutDelegate.BindUObject(this, &AHostile::HandleAttackMontageBlendingOut);
    AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);

    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &AHostile::HandleAttackMontageEnded);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

    return true;
}

void AHostile::HandleAttackMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
    CompleteAttackState(Montage, bInterrupted, TEXT("BlendingOut"));
}

void AHostile::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    CompleteAttackState(Montage, bInterrupted, TEXT("Ended"));
}

void AHostile::CompleteAttackState(
    UAnimMontage* Montage,
    bool bInterrupted,
    const TCHAR* EventSource
)
{
    if (Montage && CurrentAttackMontage != Montage)
    {
        return;
    }

    const FName FinishedAttackType = CurrentHostileAttackType;
    UAnimMontage* FinishedMontage = Montage ? Montage : CurrentAttackMontage.Get();
    const bool bShouldFinishWeaponAttack = bWeaponAttackCycleActive;

    GetWorldTimerManager().ClearTimer(ConvertedGuardFinishTimer);

    /*
     * 普通攻击和转换 Guard 走不同生命周期：
     *
     * 1. 普通攻击：
     *    NotifyWeaponAttackStarted 已经把武器切入攻击周期。
     *    结束时必须调用 NotifyWeaponAttackFinished。
     *    这样 AWeaponBase::CompleteAttackCycle 才能统一清理：
     *    - Trace
     *    - 武器攻击状态
     *    - 当前攻击方向
     *    - 当前攻击数据
     *
     * 2. 转换 Guard / 播放失败：
     *    没有开启武器攻击周期。
     *    这里只做防御性关闭 Trace，不调用 NotifyWeaponAttackFinished。
     */
    if (bShouldFinishWeaponAttack)
    {
        DisableWeaponTrace();
        NotifyWeaponAttackFinished(bInterrupted);
        bWeaponAttackCycleActive = false;
    }
    else
    {
        DisableWeaponTrace();
    }

    CurrentAttackMontage = nullptr;
    bIsAttacking = false;
    bCanAttack = true;
    CurrentAttackDamage = 0.0f;
    CurrentHostileAttackDirection = EAttackDirection::None;
    CurrentHostileAttackType = NAME_None;
    CurrentHostileAttackStartTime = 0.0f;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[敌人攻击][状态清理] 敌人=%s AttackID=%s Montage=%s 来源=%s 是否中断=%s"),
        *GetName(),
        *FinishedAttackType.ToString(),
        *GetNameSafe(FinishedMontage),
        EventSource,
        bInterrupted ? TEXT("是") : TEXT("否")
    );
}

void AHostile::FinishConvertedGuard()
{
    CompleteAttackState(nullptr, false, TEXT("ConvertedGuardTimer"));
}

void AHostile::CancelActiveAttack(const FString& Reason)
{
    UAnimMontage* MontageToStop = CurrentAttackMontage.Get();

    CompleteAttackState(MontageToStop, true, TEXT("CancelAttack"));

    if (MontageToStop)
    {
        FAttackAnimationPlayer::StopAttackMontage(this, MontageToStop, 0.10f);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[敌人攻击][统一取消] 敌人=%s 原因=%s"),
        *GetName(),
        *Reason
    );
}

void AHostile::OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[敌人攻击][被格挡取消] 敌人=%s 格挡者=%s 原因=%s"),
        *GetName(),
        GuardActor ? *GuardActor->GetName() : TEXT("无"),
        *Reason
    );
}