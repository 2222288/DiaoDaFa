#include "Character/Hostile.h"

#include "AnimationLogic/AttackAnimationPlayer.h"
#include "Blueprint/UserWidget.h"
#include "Widget/EnemyHealthBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
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

const FAttack* AHostile::GetRandomAttackData() const
{
	if (!AttackDataTable) return nullptr;

	TArray<FAttack*> AllRows;
	AttackDataTable->GetAllRows<FAttack>(TEXT("AHostile::GetRandomAttackData"), AllRows);

	if (AllRows.Num() <= 0) return nullptr;

	const int32 RandomIndex = FMath::RandRange(0, AllRows.Num() - 1);
	return AllRows[RandomIndex];
}

void AHostile::Attack()
{
	if (!CanAttack())
	{
		return;
	}

	const FAttack* AttackData = GetRandomAttackData();
	if (!AttackData || !AttackData->AttackMontage)
	{
		return;
	}

	const FAttackAnimationPlayResult AnimationResult =
		FAttackAnimationPlayer::PlayAttackMontage(this, *AttackData, 1.0f);

	if (!AnimationResult.bSucceeded)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("敌人攻击动画播放失败: Reason=%s, Montage=%s, Section=%s"),
			*AnimationResult.ErrorMessage,
			*GetNameSafe(AttackData->AttackMontage),
			*AttackData->MontageSection.ToString()
		);
		return;
	}

	bIsAttacking = true;
	bCanAttack = false;

	CurrentAttackDamage = AttackData->Damage;
	CurrentHostileAttackDirection = AttackData->AttackDirection;
	CurrentHostileAttackType = AttackData->AttackID;
	CurrentHostileAttackStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	NotifyWeaponAttackStarted(
		CurrentHostileAttackDirection,
		CurrentHostileAttackType,
		CurrentHostileAttackStartTime,
		AttackData->Damage,
		1.0f
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
		TEXT("敌人发起攻击: AttackID=%s, Section=%s, Damage=%f, Duration=%f"),
		*CurrentHostileAttackType.ToString(),
		*AnimationResult.PlayedSection.ToString(),
		CurrentAttackDamage,
		AttackDuration
	);
}

void AHostile::FinishAttack()
{
	DisableWeaponTrace();

	bIsAttacking = false;
	bCanAttack = true;

	UE_LOG(LogTemp, Warning, TEXT("敌人攻击动作结束"));
}