#include "Character/Hostile.h"

#include "Blueprint/UserWidget.h"
#include "Widget/EnemyHealthBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "Animation/AnimInstance.h"
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
	// 没在冷却且没在攻击中才能攻击
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

float AHostile::GetMontageSectionLength(UAnimMontage* Montage, FName SectionName) const
{
	if (!Montage || SectionName == NAME_None) return 0.0f;

	const int32 SectionIndex = Montage->GetSectionIndex(SectionName);
	if (SectionIndex == INDEX_NONE) return 0.0f;

	float StartTime = 0.0f;
	float EndTime = 0.0f;
	Montage->GetSectionStartAndEndTime(SectionIndex, StartTime, EndTime);

	return FMath::Max(0.0f, EndTime - StartTime);
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

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	bIsAttacking = true;
	bCanAttack = false;

	AnimInstance->Montage_Play(AttackData->AttackMontage);

	if (AttackData->MontageSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(
			AttackData->MontageSection,
			AttackData->AttackMontage
		);
	}

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

	float AttackDuration = GetMontageSectionLength(
		AttackData->AttackMontage,
		AttackData->MontageSection
	);

	if (AttackDuration <= 0.0f)
	{
		AttackDuration = AttackData->AttackMontage->GetPlayLength();
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
		TEXT("敌人发起攻击: Section=%s, Damage=%f, Duration=%f"),
		*AttackData->MontageSection.ToString(),
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