// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/BaseAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UBaseAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());

	// 缓存组件
    if (OwnerCharacter)
    {
        MovementComponent = OwnerCharacter->GetCharacterMovement();
        CombatComponent = OwnerCharacter->FindComponentByClass<UCombatComponent>();
    }

}

void UBaseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 防御性编程：如果角色还没加载出来，就别瞎算
    if (!OwnerCharacter || !MovementComponent)
    {
        return;
    }



    // 1计算地面速度
    Velocity = OwnerCharacter->GetVelocity();
    FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
    GroundSpeed = LateralVelocity.Size(); // VSizeXY

    // 判断是否移动
    bIsMoving = GroundSpeed > 3.0f;

    // 判断是否在空中
    bIsFalling = MovementComponent->IsFalling();

    if (bIsMoving)
    {
        // 获取角色当前的旋转朝向
        FRotator ActorRotation = OwnerCharacter->GetActorRotation();

        // 把世界速度向量，逆向旋转回角色的局部坐标系
        FVector LocalVelocity = ActorRotation.UnrotateVector(Velocity);

        //直接提取这个局部向量的方向角 (Yaw)
        Direction = LocalVelocity.ToOrientationRotator().Yaw;
    }
    else
    {
        Direction = 0.0f;
    }

    if (CombatComponent)
    {
        AttackState = CombatComponent->GetAttackState();
        AttackDirection = CombatComponent->GetCurrentDirection();
        bIsAttackActive = CombatComponent->IsAttackActive();
        AttackTriggerCounter = CombatComponent->GetAttackTriggerCounter();

        bAttackTriggered = AttackTriggerCounter != LastAttackTriggerCounter;
        if (bAttackTriggered)
        {
            LastAttackTriggerCounter = AttackTriggerCounter;
        }
    }
    else
    {
        AttackState = EAttackState::Idle;
        AttackDirection = EAttackDirection::None;
        bIsAttackActive = false;
        bAttackTriggered = false;
        AttackTriggerCounter = 0;
    }

}