// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/MyPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/My.h"
#include "Components/LockOn.h"



AMyPlayerController::AMyPlayerController()
{
    Lock = CreateDefaultSubobject<ULockOn>(TEXT("LockOn"));

}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }


}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // Ëø¶¨
        if (Lock)
        {
            EIC->BindAction(Lockbutton, ETriggerEvent::Started, this, &AMyPlayerController::OnToggleLockOn);
            EIC->BindAction(LLock, ETriggerEvent::Started, this, &AMyPlayerController::OnSwitchTargetLeft);
            EIC->BindAction(RLock, ETriggerEvent::Started, this, &AMyPlayerController::OnSwitchTargetRight);
        }

    }
}

void AMyPlayerController::OnToggleLockOn()
{
    if (AMy* My = Cast<AMy>(GetCharacter())) {
        if (ULockOn* LockComp = My->FindComponentByClass<ULockOn>())
        {
            LockComp->ToggleLockOn();
        }
    }
}

void AMyPlayerController::OnSwitchTargetLeft()
{
    if (AMy* My = Cast<AMy>(GetCharacter())) {
        if (ULockOn* LockComp = My->FindComponentByClass<ULockOn>())
        {
            LockComp->SwitchTargetLeft();
        }
    }
}

void AMyPlayerController::OnSwitchTargetRight()
{
    if (AMy* My = Cast<AMy>(GetCharacter())) {
        if (ULockOn* LockComp = My->FindComponentByClass<ULockOn>())
        {
            LockComp->SwitchTargetRight();
        }
    }
}




