#include "Character/Base.h"
#include "Weapon/WeaponBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AttackComponent.h"
#include "AnimationLogic/AttackAnimationPlayer.h"
#include "DataAsset/CombatReactionAnimationData.h"
#include "Character/Hostile.h"
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

    if (CurrentHealth > 0.0f)
    {
        PlayHitReaction();
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

    if (UAttackComponent* AttackComponent = FindComponentByClass<UAttackComponent>())
    {
        AttackComponent->InterruptCurrentAttack();
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

    const bool bSelfIsSlower =
        ResolveOutput.SlowerSide == SelfSide ||
        ResolveOutput.SlowerSide == EWeaponContactSide::Both;

    ECombatReactionType ReactionType = ECombatReactionType::None;

    switch (ResolveOutput.Result)
    {
    case EWeaponContactResult::Clash:
        // 敌方攻击时间比我早，我方是较慢方，并且仍在有效响应窗口内；
        // 此时我方播放格挡/架招动画。
        if (bSelfIsSlower && ResolveOutput.bIsValidTimedResponse)
        {
            ReactionType = ECombatReactionType::Guard;
        }
        else
        {
            ReactionType = ECombatReactionType::Clash;
        }
        break;

    case EWeaponContactResult::Deflect:
        ReactionType = ECombatReactionType::Deflect;
        break;

    case EWeaponContactResult::Interrupt:
        ReactionType = ECombatReactionType::Interrupt;
        break;

    case EWeaponContactResult::Hit:
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