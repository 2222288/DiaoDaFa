// Fill out your copyright notice in the Description page of Project Settings.



#include "GameFramework/MyGameModeBase.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerController.h"


AMyGameModeBase::AMyGameModeBase()
{
	GameStateClass = AMyGameStateBase::StaticClass();              // 中文注释：指定全局游戏状态类
	PlayerControllerClass = AMyPlayerController::StaticClass(); // 中文注释：指定玩家控制器类

}