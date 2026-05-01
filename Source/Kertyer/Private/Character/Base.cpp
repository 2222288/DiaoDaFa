// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Base.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
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

    //血量设置
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
}

void ABase::BeginPlay() {
    Super::BeginPlay();

}

//治疗函数实现
void ABase::Treat(float Treatmentamount) {
    // 1. 防御性编程：如果是负数治疗（毒奶？）或者血已经满了，直接忽略
    if (Treatmentamount <= 0.0f || CurrentHealth >= MaxHealth)
    {
        return;
    }

    // 2. 核心逻辑：加血，但最高不能超过 MaxHealth
    // FMath::Clamp 确保结果稳稳地落在 [0, MaxHealth] 区间内
    CurrentHealth = FMath::Clamp(CurrentHealth + Treatmentamount, 0.0f, MaxHealth);

    // 3. 打印日志：看着舒服
    UE_LOG(LogTemp, Warning, TEXT(">> [治疗] %s 回复了 %f 点生命，当前血量: %f"), *GetName(), Treatmentamount, CurrentHealth);
}

// 重写受伤函数 DamageAmount伤害
float ABase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) {
    // 1. 调用父类逻辑（虽然ACharacter默认没干啥，但保留好习惯）
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 防御性编程：如果血已经空了，就别再扣了，或者伤害是负数也不管
    if (CurrentHealth <= 0.0f || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    // 2. 核心扣血公式：当前血量 - 实际伤害
    // FMath::Clamp 确保血量不会变成负数，最小是 0
    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);

    // 3. 打印日志（方便领导您调试，看到到底扣了多少血）
    // %s 是名字，%f 是浮点数
    UE_LOG(LogTemp, Warning, TEXT("角色 %s 受到 %f 点伤害，剩余血量: %f"), *GetName(), DamageAmount, CurrentHealth);

    // 4. 判断死亡
    if (CurrentHealth <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("角色 %s 已死亡！"), *GetName());
        // 这里以后可以加：播放死亡动画、掉落装备、销毁Actor等
        // Destroy(); 
    }

    return ActualDamage;
}