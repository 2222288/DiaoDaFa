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
        return;
    }

    FWeaponAttackData AttackData;
    AttackData.AttackDirection = AttackDirection;
    AttackData.AttackStartTime = AttackStartTime;
    AttackData.AttackType = AttackType;
    AttackData.BaseDamage = BaseDamage;
    AttackData.DamageModifier = DamageModifier;

    CurrentWeapon->ReceiveAttackData(AttackData);
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
        return 0.0f;
    }

    const float ActualDamage = FMath::Min(CurrentHealth, DamageAmount);
    Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
    UE_LOG(LogTemp, Warning, TEXT("角色 %s 受到 %f 点伤害，剩余血量: %f"), *GetName(), ActualDamage, CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("角色 %s 已死亡！"), *GetName());
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
