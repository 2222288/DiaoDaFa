#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponContactResolver.h"
#include "Character/Base.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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

	const TCHAR* ContactResultToText(EWeaponContactResult Result)
	{
		switch (Result)
		{
		case EWeaponContactResult::Clash: return TEXT("武器反馈");
		case EWeaponContactResult::Deflect: return TEXT("偏斜");
		case EWeaponContactResult::Interrupt: return TEXT("打断");
		case EWeaponContactResult::Hit: return TEXT("造成伤害");
		case EWeaponContactResult::Ignore: return TEXT("忽略");
		default: return TEXT("未知");
		}
	}

	const TCHAR* RelationToText(EWeaponContactDirectionRelation Relation)
	{
		switch (Relation)
		{
		case EWeaponContactDirectionRelation::Invalid: return TEXT("无效");
		case EWeaponContactDirectionRelation::Opposite: return TEXT("对向攻击");
		case EWeaponContactDirectionRelation::NearOpposite: return TEXT("较对向攻击");
		case EWeaponContactDirectionRelation::NonOpposite: return TEXT("非对向攻击");
		default: return TEXT("未知");
		}
	}

	const TCHAR* SideToText(EWeaponContactSide Side)
	{
		switch (Side)
		{
		case EWeaponContactSide::None: return TEXT("无");
		case EWeaponContactSide::WeaponA: return TEXT("A方");
		case EWeaponContactSide::WeaponB: return TEXT("B方");
		case EWeaponContactSide::Both: return TEXT("双方");
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

	AWeaponBase* FindWeaponFromHit(const FHitResult& Hit)
	{
		if (AWeaponBase* Weapon = Cast<AWeaponBase>(Hit.GetActor()))
		{
			return Weapon;
		}

		if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
		{
			return Cast<AWeaponBase>(HitComponent->GetOwner());
		}

		return nullptr;
	}

	ABase* FindBodyFromHit(const FHitResult& Hit)
	{
		if (ABase* Body = Cast<ABase>(Hit.GetActor()))
		{
			return Body;
		}

		if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
		{
			return Cast<ABase>(HitComponent->GetOwner());
		}

		return nullptr;
	}
}

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

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
	WeaponCollision->SetGenerateOverlapEvents(true);
	WeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
	WeaponCollision->SetSimulatePhysics(false);
	WeaponCollision->SetEnableGravity(false);

	BladeSocketNames = { TEXT("Blade_Base"), TEXT("Blade_Mid"), TEXT("Blade_Tip") };
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentWeaponState = EWeaponState::Idle;
	bIsTracing = false;
	bHasResolvedInteractionThisTrace = false;
	ConsumedActorThisTrace.Reset();

	ResetSocketTracePositions();
}

void AWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsTracing && !HasConsumedInteraction())
	{
		PerformSocketSweeps(DeltaSeconds);
	}
}

void AWeaponBase::SetCurrentHolder(ABase* NewHolder)
{
	CurrentHolder = NewHolder;
	SetOwner(NewHolder);
}

void AWeaponBase::ReceiveAttackData(const FWeaponAttackData& AttackData)
{
	CurrentAttackData = AttackData;
	CurrentAttackDirection = AttackData.AttackDirection;
	CurrentWeaponState = EWeaponState::Attacking;

	HitActorsThisTrace.Empty();
	ContactedWeaponsThisTrace.Empty();
	IgnoredBodyActorsThisTraceForLog.Empty();
	bHasResolvedInteractionThisTrace = false;
	ConsumedActorThisTrace.Reset();

	if (CurrentAttackData.AttackStartTime < 0.0f)
	{
		if (const UWorld* World = GetWorld())
		{
			CurrentAttackData.AttackStartTime = World->GetTimeSeconds();
		}
	}

	CurrentAttackData.CounterAttackValidWindow = NormalizeWindow(CurrentAttackData.CounterAttackValidWindow);

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

	bIsTracing = true;
	CurrentWeaponState = EWeaponState::ContactWindowOpen;

	HitActorsThisTrace.Empty();
	ContactedWeaponsThisTrace.Empty();
	IgnoredBodyActorsThisTraceForLog.Empty();
	bHasResolvedInteractionThisTrace = false;
	ConsumedActorThisTrace.Reset();

	ResetSocketTracePositions();

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][判定窗口打开] 持有者=%s 武器=%s 方向=%s 攻击ID=%s 最终伤害=%.2f 响应窗口=%.3f Socket数量=%d 半径=%.2f"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		DirectionToText(CurrentAttackDirection),
		*SafeNameText(CurrentAttackData.AttackType),
		GetCurrentAttackDamage(),
		CurrentAttackData.CounterAttackValidWindow,
		BladeSocketNames.Num(),
		TraceSphereRadius);
}

void AWeaponBase::DisableWeaponTrace()
{
	bIsTracing = false;
	PreviousSocketLocations.Empty();

	if (CurrentWeaponState != EWeaponState::Disabled)
	{
		CurrentWeaponState = EWeaponState::Recovering;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][判定窗口关闭] 持有者=%s 武器=%s 已完成交互=%s 已交互对象=%s 身体命中数=%d 武器接触数=%d 状态=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		bHasResolvedInteractionThisTrace ? TEXT("是") : TEXT("否"),
		*SafeObjectName(ConsumedActorThisTrace.Get()),
		HitActorsThisTrace.Num(),
		ContactedWeaponsThisTrace.Num(),
		WeaponStateToText(CurrentWeaponState));
}

void AWeaponBase::ForceStopWeaponInteraction(const FString& Reason)
{
	bIsTracing = false;
	bHasResolvedInteractionThisTrace = true;
	PreviousSocketLocations.Empty();

	if (CurrentWeaponState != EWeaponState::Disabled)
	{
		CurrentWeaponState = EWeaponState::Disabled;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器被强制停止] 持有者=%s 武器=%s 原因=%s 状态=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		*Reason,
		WeaponStateToText(CurrentWeaponState));
}

void AWeaponBase::ResetSocketTracePositions()
{
	PreviousSocketLocations.Empty();

	if (!WeaponMesh)
	{
		return;
	}

	for (const FName& SocketName : BladeSocketNames)
	{
		if (WeaponMesh->DoesSocketExist(SocketName))
		{
			PreviousSocketLocations.Add(SocketName, WeaponMesh->GetSocketLocation(SocketName));
		}
	}
}

void AWeaponBase::PerformSocketSweeps(float DeltaSeconds)
{
	(void)DeltaSeconds;

	if (!WeaponMesh || !GetWorld() || HasConsumedInteraction())
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponSocketSweep), false, this);
	QueryParams.AddIgnoredActor(this);

	if (CurrentHolder)
	{
		QueryParams.AddIgnoredActor(CurrentHolder);
	}

	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceSphereRadius);

	for (const FName& SocketName : BladeSocketNames)
	{
		if (HasConsumedInteraction())
		{
			return;
		}

		if (!WeaponMesh->DoesSocketExist(SocketName))
		{
			continue;
		}

		const FVector CurrentLocation = WeaponMesh->GetSocketLocation(SocketName);
		const FVector* PreviousLocationPtr = PreviousSocketLocations.Find(SocketName);

		if (!PreviousLocationPtr)
		{
			PreviousSocketLocations.Add(SocketName, CurrentLocation);
			continue;
		}

		const FVector PreviousLocation = *PreviousLocationPtr;
		PreviousSocketLocations[SocketName] = CurrentLocation;

		if (FVector::DistSquared(PreviousLocation, CurrentLocation) <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		TArray<FHitResult> Hits;
		const bool bHit = GetWorld()->SweepMultiByObjectType(
			Hits,
			PreviousLocation,
			CurrentLocation,
			FQuat::Identity,
			ObjectQueryParams,
			SphereShape,
			QueryParams
		);

		if (!bHit)
		{
			continue;
		}

		// 同一帧同时扫到身体和武器时，优先做武器交互。
		// 这样能满足“响应窗口内身体碰撞无视，只能对武器碰撞反馈”的要求。
		for (const FHitResult& Hit : Hits)
		{
			if (HasConsumedInteraction())
			{
				return;
			}

			AWeaponBase* OtherWeapon = FindWeaponFromHit(Hit);
			if (OtherWeapon)
			{
				HandleWeaponHit(OtherWeapon, Hit);
			}
		}

		for (const FHitResult& Hit : Hits)
		{
			if (HasConsumedInteraction())
			{
				return;
			}

			ABase* HitBody = FindBodyFromHit(Hit);
			if (HitBody)
			{
				HandleBodyHit(HitBody, Hit);
			}
		}
	}
}

void AWeaponBase::ProcessSweepHit(const FHitResult& Hit)
{
	if (HasConsumedInteraction())
	{
		return;
	}

	if (AWeaponBase* OtherWeapon = FindWeaponFromHit(Hit))
	{
		HandleWeaponHit(OtherWeapon, Hit);
		return;
	}

	if (ABase* HitBody = FindBodyFromHit(Hit))
	{
		HandleBodyHit(HitBody, Hit);
	}
}

void AWeaponBase::HandleBodyHit(ABase* HitBody, const FHitResult& Hit)
{
	if (!bIsTracing || HasConsumedInteraction())
	{
		return;
	}

	if (!HitBody || HitBody == CurrentHolder || HitActorsThisTrace.Contains(HitBody))
	{
		return;
	}

	float WindowElapsed = 0.0f;
	float WindowLength = 0.0f;
	FString IgnoreReason;

	if (ShouldIgnoreBodyHitByCounterWindow(HitBody, Hit, WindowElapsed, WindowLength, IgnoreReason))
	{
		const TWeakObjectPtr<ABase> WeakHitBody(HitBody);

		if (!IgnoredBodyActorsThisTraceForLog.Contains(WeakHitBody))
		{
			IgnoredBodyActorsThisTraceForLog.Add(WeakHitBody);

			UE_LOG(LogTemp, Warning,
				TEXT("[攻击交互][身体命中无视] 攻击者=%s 武器=%s 被扫到身体=%s 原因=%s 已经过=%.3f 有效窗口=%.3f 说明=窗口内只允许武器碰撞反馈，不消费本次攻击"),
				*SafeObjectName(CurrentHolder),
				*GetName(),
				*SafeObjectName(HitBody),
				*IgnoreReason,
				WindowElapsed,
				WindowLength);
		}

		return;
	}

	const float Damage = GetCurrentAttackDamage();
	ApplyBodyDamageAndInterrupt(HitBody, this, Hit, Damage, TEXT("身体被武器直接命中，本次攻击被打断"));
}

void AWeaponBase::HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit)
{
	if (!bIsTracing || HasConsumedInteraction())
	{
		return;
	}

	if (!OtherWeapon || OtherWeapon == this || OtherWeapon->CurrentHolder == CurrentHolder || OtherWeapon->HasConsumedInteraction())
	{
		return;
	}

	if (ContactedWeaponsThisTrace.Contains(OtherWeapon))
	{
		return;
	}

	ContactedWeaponsThisTrace.Add(OtherWeapon);
	OtherWeapon->ContactedWeaponsThisTrace.Add(this);

	const FWeaponContactResolveOutput ResolveOutput = UWeaponContactResolver::ResolveWeaponContactDetailed(this, OtherWeapon);

	MarkInteractionConsumed(OtherWeapon);
	OtherWeapon->MarkInteractionConsumed(this);

	ApplyContactResultToWeapons(OtherWeapon, ResolveOutput.Result);

	BP_OnWeaponContact(OtherWeapon, ResolveOutput.Result, Hit);
	OtherWeapon->BP_OnWeaponContact(this, ResolveOutput.Result, Hit);

	if (ResolveOutput.bShouldDamageSlowerBody)
	{
		AWeaponBase* DamagedWeapon = nullptr;
		AWeaponBase* DamageSourceWeapon = nullptr;

		if (ResolveOutput.DamagedSide == EWeaponContactSide::WeaponA)
		{
			DamagedWeapon = this;
			DamageSourceWeapon = OtherWeapon;
		}
		else if (ResolveOutput.DamagedSide == EWeaponContactSide::WeaponB)
		{
			DamagedWeapon = OtherWeapon;
			DamageSourceWeapon = this;
		}

		if (DamagedWeapon && DamageSourceWeapon && DamagedWeapon->CurrentHolder)
		{
			const FString Reason = FString::Printf(
				TEXT("%s：较慢方身体直接受伤并打断攻击"),
				RelationToText(ResolveOutput.DirectionRelation)
			);

			DamageSourceWeapon->ApplyBodyDamageAndInterrupt(
				DamagedWeapon->CurrentHolder,
				DamageSourceWeapon,
				Hit,
				DamageSourceWeapon->GetCurrentAttackDamage(),
				Reason
			);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器碰撞结算] A角色=%s A武器=%s A方向=%s A开始=%.3f A状态=%s A伤害=%.2f | B角色=%s B武器=%s B方向=%s B开始=%.3f B状态=%s B伤害=%.2f | 关系=%s 结果=%s 时间差=%.3f 有效窗口=%.3f 有效响应=%s 较快方=%s 较慢方=%s 优势方=%s 受伤方=%s 命中点=%s"),
		*SafeObjectName(CurrentHolder),
		*GetName(),
		DirectionToText(CurrentAttackDirection),
		CurrentAttackData.AttackStartTime,
		WeaponStateToText(CurrentWeaponState),
		GetCurrentAttackDamage(),
		*SafeObjectName(OtherWeapon->CurrentHolder),
		*OtherWeapon->GetName(),
		DirectionToText(OtherWeapon->CurrentAttackDirection),
		OtherWeapon->CurrentAttackData.AttackStartTime,
		WeaponStateToText(OtherWeapon->CurrentWeaponState),
		OtherWeapon->GetCurrentAttackDamage(),
		RelationToText(ResolveOutput.DirectionRelation),
		ContactResultToText(ResolveOutput.Result),
		ResolveOutput.TimeDelta,
		ResolveOutput.ValidResponseWindow,
		ResolveOutput.bIsValidTimedResponse ? TEXT("是") : TEXT("否"),
		SideToText(ResolveOutput.FasterSide),
		SideToText(ResolveOutput.SlowerSide),
		SideToText(ResolveOutput.AdvantageSide),
		SideToText(ResolveOutput.DamagedSide),
		*Hit.Location.ToCompactString());
}

void AWeaponBase::ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result)
{
	if (!OtherWeapon)
	{
		return;
	}

	switch (Result)
	{
	case EWeaponContactResult::Clash:
		CurrentWeaponState = EWeaponState::Recovering;
		OtherWeapon->CurrentWeaponState = EWeaponState::Recovering;
		break;

	case EWeaponContactResult::Deflect:
		CurrentWeaponState = EWeaponState::Recovering;
		OtherWeapon->CurrentWeaponState = EWeaponState::Recovering;
		break;

	case EWeaponContactResult::Interrupt:
		CurrentWeaponState = EWeaponState::Recovering;
		OtherWeapon->CurrentWeaponState = EWeaponState::Disabled;
		break;

	case EWeaponContactResult::Hit:
		CurrentWeaponState = EWeaponState::Recovering;
		OtherWeapon->CurrentWeaponState = EWeaponState::Recovering;
		break;

	case EWeaponContactResult::Ignore:
	default:
		break;
	}

	bIsTracing = false;
	OtherWeapon->bIsTracing = false;
	PreviousSocketLocations.Empty();
	OtherWeapon->PreviousSocketLocations.Empty();
}

bool AWeaponBase::ShouldIgnoreBodyHitByCounterWindow(ABase* HitBody, const FHitResult& Hit, float& OutElapsed, float& OutWindow, FString& OutReason) const
{
	(void)Hit;

	OutElapsed = -1.0f;
	OutWindow = DefaultCounterAttackWindow;
	OutReason = TEXT("无");

	if (!GetWorld() || !CurrentAttackData.IsValid())
	{
		return false;
	}

	float FirstAttackStartTime = CurrentAttackData.AttackStartTime;
	float FirstAttackWindow = NormalizeWindow(CurrentAttackData.CounterAttackValidWindow);
	FString FirstWeaponName = GetName();

	if (HitBody && HitBody->GetCurrentWeapon())
	{
		const AWeaponBase* OtherWeapon = HitBody->GetCurrentWeapon();
		const FWeaponAttackData& OtherAttackData = OtherWeapon->GetCurrentAttackData();

		if (OtherAttackData.IsValid() && OtherAttackData.AttackStartTime < FirstAttackStartTime)
		{
			FirstAttackStartTime = OtherAttackData.AttackStartTime;
			FirstAttackWindow = NormalizeWindow(OtherAttackData.CounterAttackValidWindow);
			FirstWeaponName = OtherWeapon->GetName();
		}
	}

	const float Now = GetWorld()->GetTimeSeconds();

	OutElapsed = Now - FirstAttackStartTime;
	OutWindow = FirstAttackWindow;

	const bool bInsideWindow = OutElapsed >= 0.0f && OutElapsed <= OutWindow;

	if (bInsideWindow)
	{
		OutReason = FString::Printf(
			TEXT("先发武器=%s，当前仍在先发攻击响应窗口内"),
			*FirstWeaponName
		);
	}

	return bInsideWindow;
}

void AWeaponBase::ApplyBodyDamageAndInterrupt(ABase* TargetBody, AWeaponBase* DamageSourceWeapon, const FHitResult& Hit, float Damage, const FString& Reason)
{
	if (!TargetBody || Damage <= 0.0f)
	{
		return;
	}

	AWeaponBase* SourceWeapon = DamageSourceWeapon ? DamageSourceWeapon : this;
	ABase* SourceHolder = SourceWeapon ? SourceWeapon->CurrentHolder : CurrentHolder;
	AController* InstigatorController = SourceHolder ? SourceHolder->GetController() : nullptr;

	SourceWeapon->HitActorsThisTrace.Add(TargetBody);
	SourceWeapon->MarkInteractionConsumed(TargetBody);

	UGameplayStatics::ApplyDamage(
		TargetBody,
		Damage,
		InstigatorController,
		SourceWeapon,
		UDamageType::StaticClass()
	);

	TargetBody->InterruptCurrentAttackByBodyHit(SourceWeapon);

	SourceWeapon->BP_OnBodyHit(TargetBody, Hit, Damage);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][身体伤害并打断] 攻击者=%s 攻击武器=%s 受击者=%s 方向=%s 攻击ID=%s 伤害=%.2f 原因=%s 命中点=%s"),
		*SafeObjectName(SourceHolder),
		*SafeObjectName(SourceWeapon),
		*SafeObjectName(TargetBody),
		DirectionToText(SourceWeapon->CurrentAttackDirection),
		*SafeNameText(SourceWeapon->CurrentAttackData.AttackType),
		Damage,
		*Reason,
		*Hit.Location.ToCompactString());
}

bool AWeaponBase::HasConsumedInteraction() const
{
	return bHasResolvedInteractionThisTrace;
}

void AWeaponBase::MarkInteractionConsumed(AActor* ConsumedActor)
{
	bHasResolvedInteractionThisTrace = true;
	ConsumedActorThisTrace = ConsumedActor;

	bIsTracing = false;
	PreviousSocketLocations.Empty();

	if (CurrentWeaponState != EWeaponState::Disabled)
	{
		CurrentWeaponState = EWeaponState::Recovering;
	}
}