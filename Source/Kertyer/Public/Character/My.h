// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Base.h"
#include "InputActionValue.h"
#include "Components/CombatComponent.h"
#include "My.generated.h"

class UInputMappingContext;
class UInputAction;
class ULockOn;
class UUserWidget;

/**
 * */
UCLASS()
class KERTYER_API AMy : public ABase
{
    GENERATED_BODY()

public:

    AMy();

    // 输入映射上下文 (IMC)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputMappingContext> DefaultMappingContext;

    // 移动输入动作 (IA_Move)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputAction> MoveAction;

    // 视角输入动作 (IA_Look)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputAction> LookAction;

    // 鼠标灵敏度系数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", DisplayName = "鼠标灵敏度")
    float CursorSensitivity = 1.0f;

    //锁定组件
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class ULockOn> Lock;

    //攻击组件
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UCombatComponent> Attack;

    //左攻击键
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputAction> LAttack;

    //右攻击键
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputAction> RAttack;

    //攻击采样键
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<class UInputAction> MouseDeltaAction;

    // HUD类
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;


    //////////////////////////////////////////////////////////////////////
    //是否正在攻击
    bool ifAttack = false;


public:
    //攻击按下处理事件
    void OnAttackPressed();
    //攻击松开处理事件
    void OnAttackReleased();
    //采样处理事件
    void OnMouseDelta(const FInputActionValue& Value);

protected:

    virtual void BeginPlay() override;

    // 绑定输入函数 管理数据的流通 class向前说明 UInputComponent这个类是管理数据的流通的规则 具体在函数内部实现
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // 移动逻辑实现 FInputActionValue把传入的数据进行封装,后续读取需使用模板 
    void Move(const FInputActionValue& Value);

    // 视角逻辑实现
    void Look(const FInputActionValue& Value);



};