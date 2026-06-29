#pragma once

#include "CoreMinimal.h"
#include "Character/Base.h"
#include "InputActionValue.h"
#include "My.generated.h"

class UCombatComponent;
class UInputComponent;
class UInputAction;
class ULockOn;
class UUserWidget;

UCLASS()
class KERTYER_API AMy : public ABase
{
	GENERATED_BODY()

public:
	AMy();

	// 移动输入动作（IA_Move）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// 视角输入动作（IA_Look）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (DisplayName = "鼠标灵敏度"))
	float CursorSensitivity = 1.0f;

	// Pawn 是 LockOn 的唯一组件所有者；Controller 只负责把按键路由到这里。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULockOn> Lock;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RAttack;

	// 必须配置为 Axis2D，Modifier、Dead Zone、Scalar 等均由 Enhanced Input 处理。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseDeltaAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	void OnAttackPressed();
	void OnAttackReleased();
	void OnMouseDelta(const FInputActionValue& Value);

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

private:
	bool bIsAttackInputHeld = false;
};
