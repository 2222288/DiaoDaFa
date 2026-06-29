#include "Character/My.h"

#include "Components/CombatComponent.h"
#include "Components/LockOn.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"

AMy::AMy()
{
	Lock = CreateDefaultSubobject<ULockOn>(TEXT("LockOn"));
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatOn"));
}

void AMy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(
		EIC,
		TEXT("AMy 需要 EnhancedInputComponent，请检查 PlayerController/Input 配置")))
	{
		return;
	}

	if (ensureMsgf(MoveAction, TEXT("AMy::MoveAction 未配置")))
	{
		EIC->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&AMy::Move
		);
	}

	if (ensureMsgf(LookAction, TEXT("AMy::LookAction 未配置")))
	{
		EIC->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&AMy::Look
		);
	}

	if (!ensureMsgf(Combat, TEXT("AMy::Attack 组件创建失败")))
	{
		return;
	}

	if (ensureMsgf(LAttack, TEXT("AMy::LAttack 未配置")))
	{
		EIC->BindAction(
			LAttack,
			ETriggerEvent::Started,
			this,
			&AMy::OnAttackPressed
		);
		EIC->BindAction(
			LAttack,
			ETriggerEvent::Completed,
			this,
			&AMy::OnAttackReleased
		);
		EIC->BindAction(
			LAttack,
			ETriggerEvent::Canceled,
			this,
			&AMy::OnAttackReleased
		);
	}

	if (ensureMsgf(
		MouseDeltaAction,
		TEXT("AMy::MouseDeltaAction 未配置")))
	{
		EIC->BindAction(
			MouseDeltaAction,
			ETriggerEvent::Triggered,
			this,
			&AMy::OnMouseDelta
		);
	}
}

void AMy::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AMy::Look(const FInputActionValue& Value)
{
	if (Lock && Lock->IsLockedOn())
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AMy::OnAttackPressed()
{
	if (bIsAttackInputHeld || !Combat || !GetWorld())
	{
		return;
	}

	bIsAttackInputHeld = true;
	Combat->BeginAttackSampling(GetWorld()->GetTimeSeconds());
}

void AMy::OnAttackReleased()
{
	// Completed 与 Canceled 共用同一出口；重复调用不会重复结束采样。
	if (!bIsAttackInputHeld)
	{
		return;
	}

	bIsAttackInputHeld = false;

	if (Combat)
	{
		Combat->EndAttackSampling();
	}
}

void AMy::OnMouseDelta(const FInputActionValue& Value)
{
	if (!bIsAttackInputHeld || !Combat || !GetWorld())
	{
		return;
	}

	// 直接消费 Enhanced Input 的最终值，保留 Dead Zone、Scalar、
	// 重映射和手柄/鼠标设备抽象。
	const FVector2D Delta =
		Value.Get<FVector2D>() * CursorSensitivity;

	if (Delta.IsNearlyZero())
	{
		return;
	}

	Combat->CacheMouseInput(
		Delta,
		GetWorld()->GetTimeSeconds()
	);
}
