// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/My.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Components/AttackComponent.h"
#include "Components/LockOn.h"

AMy::AMy()
{
    Lock = CreateDefaultSubobject<ULockOn>(TEXT("LockOn"));
    Attack = CreateDefaultSubobject<UAttackComponent>(TEXT("Attack"));

}

void AMy::BeginPlay()
{
    Super::BeginPlay();
    //判断PlayerController是否为空 Cast<APlayerController>(Controller)将控制器转换为玩家控制器类并对PlayerController赋值
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        //验证本地玩家是否存在  UEnhancedInputLocalPlayerSubsystem这个类是一个变量绑定到某个输入情景映射的地址上 检查Subsystem是否有效 GetLocalPlayer()获取玩家地址
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            //使创建的输入映射上下文生效 AddMappingContext将输入映射上下文添加到子系统中 第一个参数是输入映射上下文指针 第二个参数是优先级 0为默认优先级
            //AddMappingContext调用输入情景映射到当前角色
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

}
//人物运动系统
void AMy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    //UEnhancedInputComponent把一个变量绑定到某个函数上
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMy::Move);
        EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMy::Look);

        // 攻击
        if (Attack) {
            EIC->BindAction(LAttack, ETriggerEvent::Started, this, &AMy::OnAttackPressed);
            EIC->BindAction(MouseDeltaAction, ETriggerEvent::Triggered, this, &AMy::OnMouseDelta);
            EIC->BindAction(LAttack, ETriggerEvent::Completed, this, &AMy::OnAttackReleased);

        }
    }

}


//FInputActionValue把传入的数据进行封装,后续读取需使用模板
void AMy::Move(const FInputActionValue& Value)
{
    // 检查控制器是否有效
    if (Controller != nullptr)
    {
        // 获取输入的2D向量 (X, Y),Value是FInputActionValue类型的变量 使用Get模板函数获取 FVector2D是二维向量类型
        FVector2D MovementVector = Value.Get<FVector2D>();

        // 获取控制器的旋转（即摄像机看向的方向）
        const FRotator Rotation = Controller->GetControlRotation();

        //  只保留 Yaw (水平旋转)，忽略 Pitch (上下) 和 Roll
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // 根据这个 Yaw 计算出“屏幕的前方”和“屏幕的右方”
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // 施加移动
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AMy::Look(const FInputActionValue& Value)
{
    if (Lock && Lock->IsLockedOn())
    {
        return;
    }

    FVector2D LookAxis = Value.Get<FVector2D>();
    // 未锁定才旋转镜头
    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}


void AMy::OnAttackPressed()
{
    if (Attack) {
        ifAttack = true;
        const float CurrentTime = GetWorld()->GetTimeSeconds();
        Attack->BeginAttackSampling(CurrentTime);
    }
}

void AMy::OnAttackReleased()
{
    if (Attack) {
        ifAttack = false;
        Attack->EndAttackSampling();
    }
}



void AMy::OnMouseDelta(const FInputActionValue& Value)
{
    if (ifAttack)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            float DeltaX = 0.0f;
            float DeltaY = 0.0f;
            PC->GetInputMouseDelta(DeltaX, DeltaY);
            FVector2D ScaledMouseDelta = FVector2D(DeltaX, DeltaY) * CursorSensitivity;

            const float CurrentTime = GetWorld()->GetTimeSeconds();
      
            Attack->CacheMouseInput(ScaledMouseDelta, CurrentTime);
        }
    }
}
