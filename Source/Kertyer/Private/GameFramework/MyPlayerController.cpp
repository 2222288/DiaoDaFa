#include "GameFramework/MyPlayerController.h"

#include "Character/My.h"
#include "Components/LockOn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!ensureMsgf(
		DefaultMappingContext,
		TEXT("AMyPlayerController::DefaultMappingContext 未配置")))
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!ensureMsgf(
		LocalPlayer,
		TEXT("AMyPlayerController 未找到 LocalPlayer")))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!ensureMsgf(
		Subsystem,
		TEXT("AMyPlayerController 未找到 Enhanced Input Subsystem")))
	{
		return;
	}

	if (!bMappingContextAdded)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
		bMappingContextAdded = true;
	}
}

void AMyPlayerController::EndPlay(
	const EEndPlayReason::Type EndPlayReason
)
{
	if (bMappingContextAdded && DefaultMappingContext)
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->RemoveMappingContext(DefaultMappingContext);
			}
		}

		bMappingContextAdded = false;
	}

	Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC =
		Cast<UEnhancedInputComponent>(InputComponent);
	if (!ensureMsgf(
		EIC,
		TEXT("AMyPlayerController 需要 EnhancedInputComponent")))
	{
		return;
	}

	if (ensureMsgf(
		Lockbutton,
		TEXT("AMyPlayerController::Lockbutton 未配置")))
	{
		EIC->BindAction(
			Lockbutton,
			ETriggerEvent::Started,
			this,
			&AMyPlayerController::OnToggleLockOn
		);
	}

	if (ensureMsgf(
		LLock,
		TEXT("AMyPlayerController::LLock 未配置")))
	{
		EIC->BindAction(
			LLock,
			ETriggerEvent::Started,
			this,
			&AMyPlayerController::OnSwitchTargetLeft
		);
	}

	if (ensureMsgf(
		RLock,
		TEXT("AMyPlayerController::RLock 未配置")))
	{
		EIC->BindAction(
			RLock,
			ETriggerEvent::Started,
			this,
			&AMyPlayerController::OnSwitchTargetRight
		);
	}
}

ULockOn* AMyPlayerController::GetControlledLockOn() const
{
	if (const AMy* ControlledCharacter = Cast<AMy>(GetPawn()))
	{
		return ControlledCharacter->FindComponentByClass<ULockOn>();
	}

	return nullptr;
}

void AMyPlayerController::OnToggleLockOn()
{
	if (ULockOn* LockComponent = GetControlledLockOn())
	{
		LockComponent->ToggleLockOn();
	}
}

void AMyPlayerController::OnSwitchTargetLeft()
{
	if (ULockOn* LockComponent = GetControlledLockOn())
	{
		LockComponent->SwitchTargetLeft();
	}
}

void AMyPlayerController::OnSwitchTargetRight()
{
	if (ULockOn* LockComponent = GetControlledLockOn())
	{
		LockComponent->SwitchTargetRight();
	}
}
