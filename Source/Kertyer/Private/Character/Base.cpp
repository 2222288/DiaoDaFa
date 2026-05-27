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

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), FName("hand_r_weapons"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(true);
    WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    WeaponMesh->OnComponentBeginOverlap.AddDynamic(this, &ABase::OnWeaponOverlap);

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
        return;
    }

    HitActors.Empty();
    bWeaponDamageEnabled = true;

    if (WeaponMesh)
    {
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        WeaponMesh->SetGenerateOverlapEvents(true);
        WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
        WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        WeaponMesh->SetSimulatePhysics(false);
        WeaponMesh->SetEnableGravity(false);
    }
}

void ABase::DisableWeaponTrace()
{
    if (CurrentWeapon)
    {
        CurrentWeapon->DisableWeaponTrace();
        return;
    }

    bWeaponDamageEnabled = false;

    if (WeaponMesh)
    {
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetGenerateOverlapEvents(false);
    }

    HitActors.Empty();
}

void ABase::EnableWeaponDamage()
{
    EnableWeaponTrace();
}

void ABase::DisableWeaponDamage()
{
    DisableWeaponTrace();
}

void ABase::OnWeaponOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (CurrentWeapon || !bWeaponDamageEnabled)
    {
        return;
    }

    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    ABase* HitCharacter = Cast<ABase>(OtherActor);
    if (!HitCharacter)
    {
        return;
    }

    if (HitActors.Contains(OtherActor))
    {
        return;
    }

    HitActors.Add(OtherActor);

    float FinalDamage = 10.0f;
    if (UAttackComponent* AttackComp = FindComponentByClass<UAttackComponent>())
    {
        FinalDamage = AttackComp->GetCurrentAttackDamage();
    }
    else if (AHostile* HostileAttacker = Cast<AHostile>(this))
    {
        FinalDamage = HostileAttacker->GetCurrentAttackDamage();
    }

    UGameplayStatics::ApplyDamage(
        HitCharacter,
        FinalDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    UE_LOG(LogTemp, Warning, TEXT("Legacy weapon hit %s, Damage: %f"), *HitCharacter->GetName(), FinalDamage);
}