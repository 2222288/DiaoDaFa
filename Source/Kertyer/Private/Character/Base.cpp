// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Base.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AttackComponent.h"
#include "Character/Hostile.h"
#include "EnhancedInputSubsystems.h"


ABase::ABase() {
    //生命周期管理是否开启
    PrimaryActorTick.bCanEverTick = true;

    // 创建武器并挂载 CreateDefaultSubobject对创建的WeaponMesh进行初始化 UStaticMeshComponent是静态网格体组件的指针类型 TEXT在蓝图中显示的名字
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    // 这里假设您的骨骼上有一个名为"WeaponSocket"的插槽 通过武器的指针创建静态网格体,并绑定到对应的骨骼 第一个是该静态网格体组件的指针,第二是绑定到插槽的名字
    WeaponMesh->SetupAttachment(GetMesh(), FName("hand_r_weapons"));
    // 默认关闭碰撞，防止武器在这个阶段干扰胶囊体移动
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WeaponMesh->SetGenerateOverlapEvents(true);

    WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    WeaponMesh->OnComponentBeginOverlap.AddDynamic(this, &ABase::OnWeaponOverlap);

    //血量设置
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}

void ABase::BeginPlay() {
    Super::BeginPlay();

}

//治疗函数实现
void ABase::Treat(float Treatmentamount) {
    if (Treatmentamount <= 0.0f || CurrentHealth >= MaxHealth)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + Treatmentamount, 0.0f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT(">> [治疗] %s 回复了 %f 点生命，当前血量: %f"), *GetName(), Treatmentamount, CurrentHealth);
}


float ABase::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    if (CurrentHealth <= 0.0f || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    const float ActualDamage = FMath::Min(CurrentHealth, DamageAmount);

    Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

    UE_LOG(LogTemp, Warning, TEXT("角色 %s 受到 %f 点伤害，剩余血量: %f"),
        *GetName(),
        ActualDamage,
        CurrentHealth);

    if (CurrentHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("角色 %s 已死亡！"), *GetName());
    }

    return ActualDamage;
}

void ABase::EnableWeaponDamage()
{
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

void ABase::DisableWeaponDamage()
{
    bWeaponDamageEnabled = false;

    if (WeaponMesh)
    {
        WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponMesh->SetGenerateOverlapEvents(false);
    }

    HitActors.Empty();
}

void ABase::OnWeaponOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // 不在攻击伤害窗口内，不造成伤害
    if (!bWeaponDamageEnabled)
    {
        return;
    }

    // 没有目标，或者打到自己，直接返回
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // 只允许打 ABase 或 ABase 派生类，比如 AHostile、AMy
    ABase* HitCharacter = Cast<ABase>(OtherActor);

    if (!HitCharacter)
    {
        return;
    }

    // 防止同一次攻击重复命中同一个角色
    if (HitActors.Contains(OtherActor))
    {
        return;
    }

    // 记录这个角色已经被本次攻击打中过
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

    // 对真正转换出来的 HitCharacter 造成伤害
    UGameplayStatics::ApplyDamage(
        HitCharacter,
        FinalDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    UE_LOG(LogTemp, Warning, TEXT("Weapon hit %s, Damage: %f"),
        *HitCharacter->GetName(),
        FinalDamage);
}