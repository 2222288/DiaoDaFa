// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DataAsset/AttackDH.h"
#include "BaseAnimInstance.generated.h"

class UAttackComponent;

/**
 * */
UCLASS()
class KERTYER_API UBaseAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:

    // 地面速度 (用于驱动 Idle/Walk/Run 混合)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    float GroundSpeed;

    // 是否在移动 (用于切换状态机)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

    // 是否在空中 (用于跳跃动画)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    bool bIsFalling;

    // 速度向量
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    FVector Velocity;

    //方向
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    float Direction;

    // 角色引用 (缓存起来，避免每帧 Cast，优化性能)
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<class ACharacter> OwnerCharacter;

    // 移动组件引用
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<class UCharacterMovementComponent> MovementComponent;

    // 攻击组件引用
    UPROPERTY(BlueprintReadOnly, Category = "References")
    TObjectPtr<UAttackComponent> AttackComponent;

    // 攻击状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    EAttackState AttackState = EAttackState::Idle;

    // 当前攻击方向
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    EAttackDirection AttackDirection = EAttackDirection::None;

    // 当前是否处于攻击中
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bIsAttackActive = false;

    // 本帧是否触发了新攻击
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bAttackTriggered = false;

    // 攻击触发计数
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    int32 AttackTriggerCounter = 0;

protected:
    // 初始化函数 (对应蓝图的 Event Blueprint Initialize Animation)
    virtual void NativeInitializeAnimation() override;

    // 更新函数 (对应蓝图的 Event Blueprint Update Animation)
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
    int32 LastAttackTriggerCounter = 0;

};