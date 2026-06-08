#include "Character/Hostile.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Blueprint/UserWidget.h"
#include "Widget/EnemyHealthBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DataAsset/AttackMoveDataAsset.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AHostile::AHostile()
{
	PrimaryActorTick.bCanEverTick = false;

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));

	HealthBarWidget->SetupAttachment(GetMesh(), TEXT("head"));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(200.0f, 24.0f));
	HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));

	HealthBarWidget->SetVisibility(true);
	HealthBarWidget->SetHiddenInGame(false);
	HealthBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bUseControllerRotationYaw = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}
}

void AHostile::BeginPlay()
{
	Super::BeginPlay();

	if (HealthBarWidget)
	{
		HealthBarWidget->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("head")
		);
		HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
		HealthBarWidget->InitWidget();
	}

	UpdateHealthBar();
}

float AHostile::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Result = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UpdateHealthBar();
	return Result;
}

void AHostile::UpdateHealthBar()
{
	if (!HealthBarWidget) return;

	UUserWidget* RawWidget = HealthBarWidget->GetUserWidgetObject();

	if (!RawWidget)
	{
		//主动初始化控件
		HealthBarWidget->InitWidget();
		RawWidget = HealthBarWidget->GetUserWidgetObject();
	}

	UEnemyHealthBarWidget* Widget = Cast<UEnemyHealthBarWidget>(RawWidget);
	if (Widget)
	{
		const float HealthPercent = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
		Widget->SetHealthPercent(HealthPercent);
	}
}

bool AHostile::CanAttack() const
{
	//没在冷却且没在攻击中才能攻击
	return bCanAttack && !bIsAttacking;
}

float AHostile::GetCurrentAttackDamage() const
{
	return CurrentAttackDamage;
}

const FAttackMoveData* AHostile::GetRandomAttackData() const
{
	if (!AttackMoveDataAsset)
	{
		return nullptr;
	}

	return AttackMoveDataAsset->GetRandomAttack();
}


void AHostile::Attack()
{
	if (!CanAttack())
	{
		return;
	}

	const FAttackMoveData* AttackData = GetRandomAttackData();
	if (!AttackData || !AttackData->AttackMontage)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	float GuardDuration = 0.0f;
	if (TryConvertAttackToGuard(AttackData->AttackDirection, CurrentTime, GuardDuration))
	{
		bIsAttacking = true;
		bCanAttack = false;

		CurrentAttackDamage = 0.0f;
		CurrentHostileAttackDirection = AttackData->AttackDirection;
		CurrentHostileAttackType = TEXT("Guard");
		CurrentHostileAttackStartTime = CurrentTime;

		const float SafeGuardDuration = FMath::Max(0.1f, GuardDuration);

		GetWorldTimerManager().ClearTimer(FinishAttackTimer);
		GetWorldTimerManager().SetTimer(
			FinishAttackTimer,
			this,
			&AHostile::FinishAttack,
			SafeGuardDuration,
			false
		);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("敌人攻击请求被转换为格挡: Direction=%d, Duration=%f"),
			static_cast<int32>(CurrentHostileAttackDirection),
			SafeGuardDuration
		);

		return;
	}

	const float AttackPlayRate = ConsumeNextAttackPlayRateModifier();

	const float ActualCounterAttackValidWindow =
		AttackData->CounterAttackValidWindow > 0.0f
		? AttackData->CounterAttackValidWindow
		: 0.5f;

	CurrentAttackDamage = AttackData->Damage;
	CurrentHostileAttackDirection = AttackData->AttackDirection;
	CurrentHostileAttackType = AttackData->AttackID;
	CurrentHostileAttackStartTime = CurrentTime;

	const FAttackAnimationPlayResult AnimationResult =
		FAttackAnimationPlayer::PlayAttackMontage(this, *AttackData, AttackPlayRate);

	if (!AnimationResult.bSucceeded)
	{
		if (AttackPlayRate > 1.0f)
		{
			GrantNextAttackSpeedBonus(AttackPlayRate);
		}

		CurrentAttackDamage = 0.0f;
		CurrentHostileAttackDirection = EAttackDirection::None;
		CurrentHostileAttackType = NAME_None;
		CurrentHostileAttackStartTime = 0.0f;

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("敌人攻击动画播放失败: AttackID=%s, Section=%s, PlayRate=%f, Error=%s"),
			*AttackData->AttackID.ToString(),
			*AttackData->MontageSection.ToString(),
			AttackPlayRate,
			*AnimationResult.ErrorMessage
		);

		return;
	}

	bIsAttacking = true;
	bCanAttack = false;

	NotifyWeaponAttackStarted(
		CurrentHostileAttackDirection,
		CurrentHostileAttackType,
		CurrentHostileAttackStartTime,
		AttackData->Damage,
		1.0f,
		ActualCounterAttackValidWindow
	);

	float AttackDuration = AnimationResult.PlayedLength;

	if (AttackDuration <= 0.0f && AnimationResult.PlayedMontage)
	{
		AttackDuration = AnimationResult.PlayedMontage->GetPlayLength();
	}

	if (AttackDuration <= 0.0f)
	{
		AttackDuration = 0.1f;
	}

	GetWorldTimerManager().ClearTimer(FinishAttackTimer);
	GetWorldTimerManager().SetTimer(
		FinishAttackTimer,
		this,
		&AHostile::FinishAttack,
		AttackDuration,
		false
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("敌人发起攻击: AttackID=%s, Section=%s, Damage=%f, Duration=%f, ResponseWindow=%f, PlayRate=%f"),
		*CurrentHostileAttackType.ToString(),
		*AnimationResult.PlayedSection.ToString(),
		CurrentAttackDamage,
		AttackDuration,
		ActualCounterAttackValidWindow,
		AttackPlayRate
	);
}

void AHostile::FinishAttack()
{
	DisableWeaponTrace();

	bIsAttacking = false;
	bCanAttack = true;

	UE_LOG(LogTemp, Warning, TEXT("敌人攻击动作结束"));
}

void AHostile::OnAttackCancelledByGuard(AActor* GuardActor, const FString& Reason)
{
	GetWorldTimerManager().ClearTimer(EnableDamageTimer);
	GetWorldTimerManager().ClearTimer(DisableDamageTimer);
	GetWorldTimerManager().ClearTimer(FinishAttackTimer);

	DisableWeaponTrace();

	bIsAttacking = false;
	bCanAttack = true;

	CurrentAttackDamage = 0.0f;
	CurrentHostileAttackDirection = EAttackDirection::None;
	CurrentHostileAttackType = NAME_None;
	CurrentHostileAttackStartTime = 0.0f;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[敌人攻击][被格挡取消] 敌人=%s 格挡者=%s 原因=%s"),
		*GetName(),
		GuardActor ? *GuardActor->GetName() : TEXT("无"),
		*Reason
	);
}