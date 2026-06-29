#include "Components/WeaponInteractionComponent.h"

#include "Character/Base.h"
#include "Components/PrimitiveComponent.h"
#include "Components/WeaponTraceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponContactResolver.h"

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

	void MoveActiveWeaponToRecovering(AWeaponBase* Weapon, FName Context)
	{
		if (!Weapon)
		{
			return;
		}

		const EWeaponState State = Weapon->GetWeaponState();
		if (State == EWeaponState::Attacking
			|| State == EWeaponState::ContactWindowOpen)
		{
			Weapon->TryTransitionWeaponState(EWeaponState::Recovering, Context);
		}
	}

	void DisableActiveWeapon(AWeaponBase* Weapon, FName Context)
	{
		if (!Weapon)
		{
			return;
		}

		const EWeaponState State = Weapon->GetWeaponState();
		if (State == EWeaponState::Attacking
			|| State == EWeaponState::ContactWindowOpen)
		{
			Weapon->TryTransitionWeaponState(EWeaponState::Disabled, Context);
		}
	}
}

UWeaponInteractionComponent::UWeaponInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	ResetForNewAttack();
}

void UWeaponInteractionComponent::ResetForNewAttack()
{
	HitActorsThisTrace.Empty();
	ContactedWeaponsThisTrace.Empty();
	IgnoredBodyActorsThisTraceForLog.Empty();
	bHasResolvedInteractionThisTrace = false;
	ConsumedActorThisTrace.Reset();
}

void UWeaponInteractionComponent::ResetForNewTraceWindow()
{
	ResetForNewAttack();
}

void UWeaponInteractionComponent::HandleTraceHits(const TArray<FHitResult>& Hits)
{
	for (const FHitResult& Hit : Hits)
	{
		if (HasConsumedInteraction())
		{
			break;
		}

		HandleTraceHit(Hit);
	}
}

void UWeaponInteractionComponent::HandleTraceHit(const FHitResult& Hit)
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

AWeaponBase* UWeaponInteractionComponent::GetOwnerWeapon() const
{
	return Cast<AWeaponBase>(GetOwner());
}

void UWeaponInteractionComponent::HandleBodyHit(ABase* HitBody, const FHitResult& Hit)
{
	AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	if (!OwnerWeapon || !OwnerWeapon->IsWeaponTracing() || HasConsumedInteraction())
	{
		return;
	}

	if (!HitBody || HitBody == OwnerWeapon->GetCurrentHolder() || HitActorsThisTrace.Contains(HitBody))
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
				*SafeObjectName(OwnerWeapon->GetCurrentHolder()),
				*OwnerWeapon->GetName(),
				*SafeObjectName(HitBody),
				*IgnoreReason,
				WindowElapsed,
				WindowLength);
		}

		return;
	}

	const float Damage = OwnerWeapon->GetCurrentAttackDamage();
	ApplyBodyDamageAndInterrupt(HitBody, OwnerWeapon, Hit, Damage, TEXT("身体被武器直接命中，本次攻击被打断"));
}

void UWeaponInteractionComponent::HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit)
{
	AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	if (!OwnerWeapon || !OwnerWeapon->IsWeaponTracing() || HasConsumedInteraction())
	{
		return;
	}

	UWeaponInteractionComponent* OtherInteraction = OtherWeapon ? OtherWeapon->GetWeaponInteractionComponent() : nullptr;
	if (!OtherWeapon || OtherWeapon == OwnerWeapon || OtherWeapon->GetCurrentHolder() == OwnerWeapon->GetCurrentHolder() ||
		(OtherInteraction && OtherInteraction->HasConsumedInteraction()))
	{
		return;
	}

	if (ContactedWeaponsThisTrace.Contains(OtherWeapon))
	{
		return;
	}

	ContactedWeaponsThisTrace.Add(OtherWeapon);

	if (OtherInteraction)
	{
		OtherInteraction->ContactedWeaponsThisTrace.Add(OwnerWeapon);
	}

	const FWeaponContactResolveOutput ResolveOutput = UWeaponContactResolver::ResolveWeaponContactDetailed(OwnerWeapon, OtherWeapon);

	MarkInteractionConsumed(OtherWeapon);

	if (OtherInteraction)
	{
		OtherInteraction->MarkInteractionConsumed(OwnerWeapon);
	}

	ApplyContactResultToWeapons(OtherWeapon, ResolveOutput.Result);

	if (OwnerWeapon->GetCurrentHolder())
	{
		OwnerWeapon->GetCurrentHolder()->PlayWeaponContactReaction(
			ResolveOutput,
			EWeaponContactSide::WeaponA
		);
	}

	if (OtherWeapon && OtherWeapon->GetCurrentHolder())
	{
		OtherWeapon->GetCurrentHolder()->PlayWeaponContactReaction(
			ResolveOutput,
			EWeaponContactSide::WeaponB
		);
	}

	OwnerWeapon->DispatchWeaponContactFeedback(OtherWeapon, ResolveOutput.Result, Hit);

	if (OtherWeapon)
	{
		OtherWeapon->DispatchWeaponContactFeedback(OwnerWeapon, ResolveOutput.Result, Hit);
	}

	if (ResolveOutput.bShouldDamageSlowerBody)
	{
		AWeaponBase* DamagedWeapon = nullptr;
		AWeaponBase* DamageSourceWeapon = nullptr;

		if (ResolveOutput.DamagedSide == EWeaponContactSide::WeaponA)
		{
			DamagedWeapon = OwnerWeapon;
			DamageSourceWeapon = OtherWeapon;
		}
		else if (ResolveOutput.DamagedSide == EWeaponContactSide::WeaponB)
		{
			DamagedWeapon = OtherWeapon;
			DamageSourceWeapon = OwnerWeapon;
		}

		if (DamagedWeapon && DamageSourceWeapon && DamagedWeapon->GetCurrentHolder())
		{
			const FString Reason = FString::Printf(
				TEXT("%s：较慢方身体直接受伤并打断攻击"),
				RelationToText(ResolveOutput.DirectionRelation)
			);

			const bool bFailedGuardContact =
				ResolveOutput.Result == EWeaponContactResult::Clash &&
				ResolveOutput.bIsValidTimedResponse &&
				ResolveOutput.DirectionRelation == EWeaponContactDirectionRelation::NonOpposite;

			UWeaponInteractionComponent* DamageSourceInteraction = DamageSourceWeapon->GetWeaponInteractionComponent();

			if (DamageSourceInteraction)
			{
				DamageSourceInteraction->ApplyBodyDamageAndInterrupt(
					DamagedWeapon->GetCurrentHolder(),
					DamageSourceWeapon,
					Hit,
					DamageSourceWeapon->GetCurrentAttackDamage(),
					Reason,
					bFailedGuardContact,
					!bFailedGuardContact
				);
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][武器碰撞结算] A角色=%s A武器=%s A方向=%s A开始=%.3f A状态=%s A伤害=%.2f | B角色=%s B武器=%s B方向=%s B开始=%.3f B状态=%s B伤害=%.2f | 关系=%s 结果=%s 时间差=%.3f 有效窗口=%.3f 有效响应=%s 较快方=%s 较慢方=%s 优势方=%s 受伤方=%s 命中点=%s"),
		*SafeObjectName(OwnerWeapon->GetCurrentHolder()),
		*OwnerWeapon->GetName(),
		DirectionToText(OwnerWeapon->GetCurrentAttackDirection()),
		OwnerWeapon->GetCurrentAttackData().AttackStartTime,
		WeaponStateToText(OwnerWeapon->GetWeaponState()),
		OwnerWeapon->GetCurrentAttackDamage(),
		*SafeObjectName(OtherWeapon->GetCurrentHolder()),
		*OtherWeapon->GetName(),
		DirectionToText(OtherWeapon->GetCurrentAttackDirection()),
		OtherWeapon->GetCurrentAttackData().AttackStartTime,
		WeaponStateToText(OtherWeapon->GetWeaponState()),
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

void UWeaponInteractionComponent::ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result)
{
	AWeaponBase* OwnerWeapon = GetOwnerWeapon();

	if (!OwnerWeapon || !OtherWeapon)
	{
		return;
	}

	switch (Result)
	{
	case EWeaponContactResult::Clash:
		MoveActiveWeaponToRecovering(OwnerWeapon, TEXT("ContactClash"));
		MoveActiveWeaponToRecovering(OtherWeapon, TEXT("ContactClash"));
		break;

	case EWeaponContactResult::Deflect:
		MoveActiveWeaponToRecovering(OwnerWeapon, TEXT("ContactDeflect"));
		MoveActiveWeaponToRecovering(OtherWeapon, TEXT("ContactDeflect"));
		break;

	case EWeaponContactResult::Interrupt:
		MoveActiveWeaponToRecovering(OwnerWeapon, TEXT("ContactInterruptOwner"));
		DisableActiveWeapon(OtherWeapon, TEXT("ContactInterruptTarget"));
		break;

	case EWeaponContactResult::Hit:
		MoveActiveWeaponToRecovering(OwnerWeapon, TEXT("ContactHit"));
		MoveActiveWeaponToRecovering(OtherWeapon, TEXT("ContactHit"));
		break;

	case EWeaponContactResult::Ignore:
	default:
		break;
	}

	if (UWeaponTraceComponent* OwnerTrace = OwnerWeapon->GetWeaponTraceComponent())
	{
		OwnerTrace->ForceStopTrace();
	}

	if (UWeaponTraceComponent* OtherTrace = OtherWeapon->GetWeaponTraceComponent())
	{
		OtherTrace->ForceStopTrace();
	}
}

bool UWeaponInteractionComponent::ShouldIgnoreBodyHitByCounterWindow(
	ABase* HitBody,
	const FHitResult& Hit,
	float& OutElapsed,
	float& OutWindow,
	FString& OutReason
) const
{
	(void)Hit;

	AWeaponBase* OwnerWeapon = GetOwnerWeapon();

	OutElapsed = -1.0f;
	OutWindow = DefaultCounterAttackWindow;
	OutReason = TEXT("无");

	if (!OwnerWeapon || !GetWorld() || !OwnerWeapon->GetCurrentAttackData().IsValid() || !HitBody || !HitBody->GetCurrentWeapon())
	{
		return false;
	}

	const AWeaponBase* OtherWeapon = HitBody->GetCurrentWeapon();
	const FWeaponAttackData& OtherAttackData = OtherWeapon->GetCurrentAttackData();

	// 只有被打者真的在本次攻击之后发起了响应攻击，才忽略身体命中。
	// 没有响应攻击时，不能因为还在响应窗口内就无视身体伤害。
	if (!OtherAttackData.IsValid())
	{
		return false;
	}

	if (OtherAttackData.AttackStartTime <= OwnerWeapon->GetCurrentAttackData().AttackStartTime)
	{
		return false;
	}

	const float FirstAttackStartTime = OwnerWeapon->GetCurrentAttackData().AttackStartTime;
	const float FirstAttackWindow = NormalizeWindow(OwnerWeapon->GetCurrentAttackData().CounterAttackValidWindow);
	const float Now = GetWorld()->GetTimeSeconds();

	OutElapsed = Now - FirstAttackStartTime;
	OutWindow = FirstAttackWindow;

	const bool bInsideWindow = OutElapsed >= 0.0f && OutElapsed <= OutWindow;

	if (bInsideWindow)
	{
		OutReason = FString::Printf(
			TEXT("被扫到身体的一方已经在响应窗口内发起后发攻击，身体命中暂不消费，等待武器反馈")
		);
	}

	return bInsideWindow;
}

void UWeaponInteractionComponent::ApplyBodyDamageAndInterrupt(
	ABase* TargetBody,
	AWeaponBase* DamageSourceWeapon,
	const FHitResult& Hit,
	float Damage,
	const FString& Reason,
	bool bSuppressNonLethalHitReaction,
	bool bInterruptTarget
)
{
	if (!TargetBody || Damage <= 0.0f)
	{
		return;
	}

	AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	AWeaponBase* SourceWeapon = DamageSourceWeapon ? DamageSourceWeapon : OwnerWeapon;

	if (!SourceWeapon)
	{
		return;
	}

	ABase* SourceHolder = SourceWeapon->GetCurrentHolder();
	AController* InstigatorController = SourceHolder ? SourceHolder->GetController() : nullptr;

	UWeaponInteractionComponent* SourceInteraction = SourceWeapon->GetWeaponInteractionComponent();

	if (SourceInteraction)
	{
		SourceInteraction->HitActorsThisTrace.Add(TargetBody);
		SourceInteraction->MarkInteractionConsumed(TargetBody);
	}

	if (bSuppressNonLethalHitReaction)
	{
		TargetBody->ApplyDamageWithoutNonLethalHitReaction(
			Damage,
			InstigatorController,
			SourceWeapon
		);
	}
	else
	{
		UGameplayStatics::ApplyDamage(
			TargetBody,
			Damage,
			InstigatorController,
			SourceWeapon,
			UDamageType::StaticClass()
		);
	}

	if (bInterruptTarget)
	{
		TargetBody->InterruptCurrentAttackByBodyHit(SourceWeapon);
	}

	SourceWeapon->DispatchBodyHitFeedback(TargetBody, Hit, Damage);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][身体伤害并打断] 攻击者=%s 攻击武器=%s 受击者=%s 方向=%s 攻击ID=%s 伤害=%.2f 抑制Hit反应=%s 打断目标=%s 原因=%s 命中点=%s"),
		*SafeObjectName(SourceHolder),
		*SafeObjectName(SourceWeapon),
		*SafeObjectName(TargetBody),
		DirectionToText(SourceWeapon->GetCurrentAttackDirection()),
		*SafeNameText(SourceWeapon->GetCurrentAttackData().AttackType),
		Damage,
		bSuppressNonLethalHitReaction ? TEXT("是") : TEXT("否"),
		bInterruptTarget ? TEXT("是") : TEXT("否"),
		*Reason,
		*Hit.Location.ToCompactString());
}

bool UWeaponInteractionComponent::HasConsumedInteraction() const
{
	return bHasResolvedInteractionThisTrace;
}

void UWeaponInteractionComponent::MarkInteractionConsumed(AActor* ConsumedActor)
{
	AWeaponBase* OwnerWeapon = GetOwnerWeapon();

	bHasResolvedInteractionThisTrace = true;
	ConsumedActorThisTrace = ConsumedActor;

	if (OwnerWeapon)
	{
		if (UWeaponTraceComponent* TraceComponent = OwnerWeapon->GetWeaponTraceComponent())
		{
			TraceComponent->ForceStopTrace();
		}

		MoveActiveWeaponToRecovering(
			OwnerWeapon,
			TEXT("InteractionConsumed")
		);
	}
}

void UWeaponInteractionComponent::ForceResolveInteraction(AActor* ConsumedActor)
{
	bHasResolvedInteractionThisTrace = true;
	ConsumedActorThisTrace = ConsumedActor;
}

AWeaponBase* UWeaponInteractionComponent::FindWeaponFromHit(const FHitResult& Hit) const
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

ABase* UWeaponInteractionComponent::FindBodyFromHit(const FHitResult& Hit) const
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