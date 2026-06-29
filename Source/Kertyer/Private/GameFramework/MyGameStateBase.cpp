// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFramework/MyGameStateBase.h"
#include "Net/UnrealNetwork.h"

AMyGameStateBase::AMyGameStateBase()
{
	// 中文注释：要复制属性，Actor 必须允许复制
	bReplicates = true;
}

void AMyGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyGameStateBase, Phase); // 中文注释：让 Phase 自动同步
}