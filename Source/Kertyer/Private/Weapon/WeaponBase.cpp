#include "Weapon/WeaponBase.h"

#include "Character/Base.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WeaponInteractionComponent.h"
#include "Components/WeaponTraceComponent.h"

namespace
{
	constexpr float DefaultCounterAttackWindow = 0.5f;

	const TCHAR* DirectionToText(EAttackDirection Direction)
	{
		switch (Direction)
		{
		case EAttackDirection::None: return TEXT("无");
		case EAttackDirection::Up: return TEXT("上");
		case EAttackDirection::UpRight: return TEXT("右上");
		case EAttackDirection::Right: return TEXT("右");
		case EAttackDirection::DownRight: return TEXT("右下");
		case EAttackDirection::Down: return TEXT("下");
		case EAttackDirection::DownLeft: return TEXT("左下");
		case EAttackDirection::Left: return TEXT("左");
		case EAttackDirection::UpLeft: return TEXT("左上");
		default: return TEXT("未知");
		}
	}

	const TCHAR* WeaponStateToText(EWeaponState State)
	{
		switch (State)
		{
		case EWeaponState::Idle: return TEXT("空闲");
		case EWeaponState::Attacking: return TEXT("攻击中");
		case EWeaponState::ContactWindowOpen: return TEXT("接触窗口打开");
		case EWeaponState::Recovering: return TEXT("恢复中");
		case EWeaponState::Disabled: return TEXT("失效");
		default: return TEXT("未知");
		}
	}

	FString SafeObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("无");
	}

	FString SafeNameText(FName Name)
	{
		return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
	}

	float NormalizeWindow(float Window)
	{
		return Window > 0.0f ? Window : DefaultCounterAttackWindow;
	}
}

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneRoot);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
	WeaponCollision->SetupAttachment(WeaponMesh);
	WeaponCollision->SetBoxExtent(FVector(8.0f, 4.0f, 60.0f));
	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponCollision->SetGenerateOverlapEvents(false);
	WeaponCollision->SetCollisionObjectType(WeaponObjectChannel.GetValue());
	WeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponCollision->SetCollisionResponseToChannel(
		WeaponObjectChannel.GetValue(),
		ECR_Overlap
	);
	WeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponCollision->SetSimulatePhysics(false);
	WeaponCollision->SetEnableGravity(false);

	WeaponTraceComponent =
		CreateDefaultSubobject<UWeaponTraceComponent>(TEXT("WeaponTraceComponent"));
	WeaponInteractionComponent =
		CreateDefaultSubobject<UWeaponInteractionComponent>(TEXT("WeaponInteractionComponent"));

	BladeSocketNames = { TEXT("Blade_Base"), TEXT("Blade_Mid"), TEXT("Blade_Tip") };
	WeaponTraceObjectChannels = { ECC_Pawn };
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentWeaponState = EWeaponState::Idle;
	ConfigureWeaponCollision();

	if (WeaponInteractionComponent)
	{
		WeaponInteractionComponent->ResetForNewAttack();
	}

	if (WeaponTraceComponent && WeaponInteractionComponent)
	{
		WeaponTraceComponent->OnWeaponTraceHits.AddUObject(
			WeaponInteractionComponent.Get(),
			&UWeaponInteractionComponent::HandleTraceHits
		);
	}
}

void AWeaponBase::ConfigureWeaponCollision()
{
	if (!WeaponCollision)
	{
		return;
	}

	WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponCollision->SetGenerateOverlapEvents(false);
	WeaponCollision->SetCollisionObjectType(WeaponObjectChannel.GetValue());
	WeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 允许武器之间产生查询结果。
	WeaponCollision->SetCollisionResponseToChannel(
		WeaponObjectChannel.GetValue(),
		ECR_Overlap
	);

	// 只对明确配置的可受击对象通道响应。
	for (const TEnumAsByte<ECollisionChannel> ObjectChannel
		: WeaponTraceObjectChannels)
	{
		WeaponCollision->SetCollisionResponseToChannel(
			ObjectChannel.GetValue(),
			ECR_Overlap
		);
	}
}

void AWeaponBase::SetCurrentHolder(ABase* NewHolder)
{
	CurrentHolder = NewHolder;
	SetOwner(NewHolder);
}

bool AWeaponBase::IsLegalStateTransition(
	EWeaponState FromState,
	EWeaponState ToState
) const
{
	if (FromState == ToState)
	{
		return true;
	}

	// 任意状态都允许进入强制失效态。
	if (ToState == EWeaponState::Disabled)
	{
		return true;
	}

	switch (FromState)
	{
	case EWeaponState::Idle:
		return ToState == EWeaponState::Attacking;

	case EWeaponState::Attacking:
		// 没有进入判定窗口或攻击提前结束时，也必须能够进入恢复态。
		return ToState == EWeaponState::ContactWindowOpen
			|| ToState == EWeaponState::Recovering;

	case EWeaponState::ContactWindowOpen:
		return ToState == EWeaponState::Recovering;

	case EWeaponState::Recovering:
		return ToState == EWeaponState::Idle;

	case EWeaponState::Disabled:
		return ToState == EWeaponState::Idle;

	default:
		return false;
	}
}

bool AWeaponBase::TryTransitionWeaponState(
	EWeaponState NewState,
	FName Context
)
{
	const EWeaponState OldState = CurrentWeaponState;

	if (!IsLegalStateTransition(OldState, NewState))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[武器状态机][拒绝非法转换] 武器=%s 持有者=%s %s -> %s 上下文=%s"),
			*GetName(),
			*SafeObjectName(CurrentHolder),
			WeaponStateToText(OldState),
			WeaponStateToText(NewState),
			*SafeNameText(Context)
		);
		return false;
	}

	if (OldState == NewState)
	{
		return true;
	}

	CurrentWeaponState = NewState;

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("[武器状态机][状态转换] 武器=%s %s -> %s 上下文=%s"),
		*GetName(),
		WeaponStateToText(OldState),
		WeaponStateToText(NewState),
		*SafeNameText(Context)
	);
	return true;
}

void AWeaponBase::PrepareForNewAttack()
{
	if (CurrentWeaponState == EWeaponState::Recovering
		|| CurrentWeaponState == EWeaponState::Disabled)
	{
		TryTransitionWeaponState(
			EWeaponState::Idle,
			TEXT("PrepareForNewAttack")
		);
	}
	else if (CurrentWeaponState != EWeaponState::Idle)
	{
		// 上一轮异常残留时，经由 Disabled -> Idle 收敛到合法起点。
		TryTransitionWeaponState(
			EWeaponState::Disabled,
			TEXT("ReplaceUnfinishedAttack")
		);
		TryTransitionWeaponState(
			EWeaponState::Idle,
			TEXT("PrepareForNewAttack")
		);
	}
}

void AWeaponBase::ReceiveAttackData(const FWeaponAttackData& AttackData)
{
	PrepareForNewAttack();

	if (!TryTransitionWeaponState(
		EWeaponState::Attacking,
		TEXT("ReceiveAttackData")))
	{
		return;
	}

	CurrentAttackData = AttackData;
	CurrentAttackDirection = AttackData.AttackDirection;

	if (WeaponInteractionComponent)
	{
		WeaponInteractionComponent->ResetForNewAttack();
	}

	if (CurrentAttackData.AttackStartTime < 0.0f)
	{
		if (const UWorld* World = GetWorld())
		{
			CurrentAttackData.AttackStartTime = World->GetTimeSeconds();
		}
	}

	CurrentAttackData.CounterAttackValidWindow =
		NormalizeWindow(CurrentAttackData.CounterAttackValidWindow);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器接收攻击数据] 持有者=%s 武器=%s 方向=%s 攻击ID=%s 攻击开始=%.3f 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 响应窗口=%.3f 状态=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		DirectionToText(CurrentAttackDirection),
		*SafeNameText(CurrentAttackData.AttackType),
		CurrentAttackData.AttackStartTime,
		CurrentAttackData.BaseDamage,
		CurrentAttackData.DamageModifier,
		GetCurrentAttackDamage(),
		CurrentAttackData.CounterAttackValidWindow,
		WeaponStateToText(CurrentWeaponState));
}

void AWeaponBase::EnableWeaponTrace()
{
	if (!WeaponMesh)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[攻击交互][判定窗口打开失败] 武器=%s 原因=WeaponMesh为空"),
			*GetName());
		return;
	}

	if (CurrentWeaponState == EWeaponState::ContactWindowOpen)
	{
		return;
	}

	if (!TryTransitionWeaponState(
		EWeaponState::ContactWindowOpen,
		TEXT("EnableWeaponTrace")))
	{
		return;
	}

	if (WeaponInteractionComponent)
	{
		WeaponInteractionComponent->ResetForNewTraceWindow();
	}

	if (WeaponTraceComponent)
	{
		WeaponTraceComponent->EnableTrace();
	}
}

void AWeaponBase::DisableWeaponTrace()
{
	if (WeaponTraceComponent)
	{
		WeaponTraceComponent->DisableTrace();
	}

	if (CurrentWeaponState == EWeaponState::ContactWindowOpen
		|| CurrentWeaponState == EWeaponState::Attacking)
	{
		TryTransitionWeaponState(
			EWeaponState::Recovering,
			TEXT("DisableWeaponTrace")
		);
	}

	const bool bResolved =
		WeaponInteractionComponent
		&& WeaponInteractionComponent->HasConsumedInteraction();
	const AActor* ConsumedActor =
		WeaponInteractionComponent
		? WeaponInteractionComponent->GetConsumedActor()
		: nullptr;
	const int32 BodyHitCount =
		WeaponInteractionComponent
		? WeaponInteractionComponent->GetBodyHitCount()
		: 0;
	const int32 WeaponContactCount =
		WeaponInteractionComponent
		? WeaponInteractionComponent->GetWeaponContactCount()
		: 0;

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][判定窗口关闭] 持有者=%s 武器=%s 已完成交互=%s 已交互对象=%s 身体命中数=%d 武器接触数=%d 状态=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		bResolved ? TEXT("是") : TEXT("否"),
		*SafeObjectName(ConsumedActor),
		BodyHitCount,
		WeaponContactCount,
		WeaponStateToText(CurrentWeaponState));
}

void AWeaponBase::ForceStopWeaponInteraction(const FString& Reason)
{
	if (WeaponTraceComponent)
	{
		WeaponTraceComponent->ForceStopTrace();
	}

	if (WeaponInteractionComponent)
	{
		WeaponInteractionComponent->ForceResolveInteraction(nullptr);
	}

	// 普通静止武器被碰到时不应永久停在 Disabled。
	// 只有确实处于攻击周期内的武器才进入强制失效态。
	if (CurrentWeaponState != EWeaponState::Idle)
	{
		TryTransitionWeaponState(
			EWeaponState::Disabled,
			TEXT("ForceStopWeaponInteraction")
		);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器被强制停止] 持有者=%s 武器=%s 原因=%s 状态=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		*Reason,
		WeaponStateToText(CurrentWeaponState));
}

void AWeaponBase::CompleteAttackCycle(bool bInterrupted)
{
	if (WeaponTraceComponent)
	{
		WeaponTraceComponent->ForceStopTrace();
	}

	if (CurrentWeaponState == EWeaponState::Attacking
		|| CurrentWeaponState == EWeaponState::ContactWindowOpen)
	{
		TryTransitionWeaponState(
			EWeaponState::Recovering,
			bInterrupted
				? TEXT("InterruptedToRecovering")
				: TEXT("AttackEndedToRecovering")
		);
	}

	if (CurrentWeaponState == EWeaponState::Recovering
		|| CurrentWeaponState == EWeaponState::Disabled)
	{
		TryTransitionWeaponState(
			EWeaponState::Idle,
			bInterrupted
				? TEXT("InterruptedToIdle")
				: TEXT("AttackEndedToIdle")
		);
	}

	CurrentAttackDirection = EAttackDirection::None;
	CurrentAttackData = FWeaponAttackData();
}

bool AWeaponBase::IsWeaponTracing() const
{
	return WeaponTraceComponent && WeaponTraceComponent->IsTracing();
}

bool AWeaponBase::HasResolvedInteractionThisTrace() const
{
	return WeaponInteractionComponent
		&& WeaponInteractionComponent->HasConsumedInteraction();
}

void AWeaponBase::DispatchBodyHitFeedback(
	ABase* HitBody,
	const FHitResult& Hit,
	float Damage
)
{
	BP_OnBodyHit(HitBody, Hit, Damage);
}

void AWeaponBase::DispatchWeaponContactFeedback(
	AWeaponBase* OtherWeapon,
	EWeaponContactResult Result,
	const FHitResult& Hit
)
{
	BP_OnWeaponContact(OtherWeapon, Result, Hit);
}
