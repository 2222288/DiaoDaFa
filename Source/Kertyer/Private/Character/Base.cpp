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

void ABase::NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier)
{
    if (!CurrentWeapon)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[攻击交互][武器数据下发失败] 角色=%s 原因=CurrentWeapon为空 方向=%s 攻击ID=%s 基础伤害=%.2f 倍率=%.3f"),
            *GetName(),
            BaseAttackDirectionToChinese(AttackDirection),
            *BaseSafeName(AttackType),
            BaseDamage,
            DamageModifier);
        return;
    }

    FWeaponAttackData AttackData;
    AttackData.AttackDirection = AttackDirection;
    AttackData.AttackStartTime = AttackStartTime;
    AttackData.AttackType = AttackType;
    AttackData.BaseDamage = BaseDamage;
    AttackData.DamageModifier = DamageModifier;

    CurrentWeapon->ReceiveAttackData(AttackData);

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][武器数据下发] 角色=%s 武器=%s 方向=%s 攻击ID=%s 开始时间=%.3f 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f"),
        *GetName(),
        *BaseSafeActorName(CurrentWeapon),
        BaseAttackDirectionToChinese(AttackDirection),
        *BaseSafeName(AttackType),
        AttackStartTime,
        BaseDamage,
        DamageModifier,
        FMath::Max(0.0f, BaseDamage * DamageModifier));
}

void ABase::Treat(float Treatmentamount)
{
    if (Treatmentamount <= 0.0f || CurrentHealth >= MaxHealth)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + Treatmentamount, 0.0f, MaxHealth);
    UE_LOG(LogTemp, Warning, TEXT(">> [治疗] %s 回复了 %f 点生命，当前血量: %f"), *GetName(), Treatmentamount, CurrentHealth);
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
