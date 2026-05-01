// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "MyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 
 */
UCLASS()
class KERTYER_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	AMyPlayerController();

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

public:

	//增强输入IMC
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	// 锁定组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class ULockOn> Lock;

	// 锁定键 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> Lockbutton;

	// 左切换目标键
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> LLock;

	// 右切换目标键
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> RLock;

public:

	void OnToggleLockOn();
	void OnSwitchTargetLeft();
	void OnSwitchTargetRight();

};
