// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MyGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class KERTYER_API AMyGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AMyGameStateBase();

	// 示例：全局阶段（比如探索/战斗/结算）
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State")
	int32 Phase = 0; // 中文注释：全局阶段，联网时会同步到客户端

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
