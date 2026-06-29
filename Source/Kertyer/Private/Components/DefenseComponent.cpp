#include "Components/DefenseComponent.h"
#include "Character/Base.h"
#include "Combat/CombatDirectionUtils.h"
#include "Components/CombatComponent.h"
#include "Components/LockOn.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Weapon/WeaponBase.h"

UDefenseComponent::UDefenseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDefenseComponent::StartDeflect()
{
	if (bIsDeflecting)
	{
		return;
	}

	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase)
	{
		return;
	}

	bIsDeflecting = true;

	if (AWeaponBase* CurrentWeapon = OwnerBase->GetCurrentWeapon())
	{
		CurrentWeapon->ForceStopWeaponInteraction(TEXT("进入弹刀状态，停止当前武器交互"));
	}

	if (UCombatComponent* FoundCombatComponent = OwnerBase->FindComponentByClass<UCombatComponent>())
	{
		FoundCombatComponent->InterruptCurrentAttack();
	}

	UE_LOG(LogTemp, Warning, TEXT("[攻击交互][弹刀开始] 角色=%s"), *OwnerBase->GetName());
}

void UDefenseComponent::EndDeflect()
{
	if (!bIsDeflecting)
	{
		return;
	}

	ABase* OwnerBase = Cast<ABase>(GetOwner());
	bIsDeflecting = false;

	UE_LOG(LogTemp, Warning, TEXT("[攻击交互][弹刀结束] 角色=%s"), *GetNameSafe(OwnerBase));
}

ABase* UDefenseComponent::FindPreAttackGuardOpponent() const
{
	const ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase)
	{
		return nullptr;
	}

	const float SearchRadius = FMath::Max(0.0f, PreAttackGuardSearchRadius);
	const float SearchRadiusSq = FMath::Square(SearchRadius);
	const FVector OwnerLocation = OwnerBase->GetActorLocation();

	const auto IsValidCandidate = [this, OwnerBase, OwnerLocation, SearchRadiusSq](ABase* Candidate) -> bool
	{
		if (!Candidate || Candidate == OwnerBase || Candidate->IsDead())
		{
			return false;
		}

		if (FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation()) > SearchRadiusSq)
		{
			return false;
		}

		const AWeaponBase* CandidateWeapon = Candidate->GetCurrentWeapon();
		if (!CandidateWeapon)
		{
			return false;
		}

		const FWeaponAttackData& CandidateAttackData = CandidateWeapon->GetCurrentAttackData();
		return CandidateAttackData.IsValid() &&
			IsWeaponInActiveAttackState(CandidateWeapon->GetWeaponState());
	};

	// 锁定目标优先，但必须重新验证距离、存活状态和攻击状态，避免远距离错误转格挡。
	if (const ULockOn* LockOn = OwnerBase->FindComponentByClass<ULockOn>())
	{
		if (ABase* LockedTarget = Cast<ABase>(LockOn->GetLockTarget()))
		{
			if (IsValidCandidate(LockedTarget))
			{
				return LockedTarget;
			}
		}
	}

	UWorld* World = OwnerBase->GetWorld();
	if (!World || SearchRadius <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PreAttackGuardSearch), false, OwnerBase);
	QueryParams.AddIgnoredActor(OwnerBase);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		Overlaps,
		OwnerLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SearchRadius),
		QueryParams);

	if (!bHasOverlap)
	{
		return nullptr;
	}

	ABase* BestTarget = nullptr;
	float BestDistSq = SearchRadiusSq;
	TSet<ABase*> SeenCandidates;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ABase* Candidate = Cast<ABase>(Overlap.GetActor());
		if (!Candidate || SeenCandidates.Contains(Candidate))
		{
			continue;
		}
		SeenCandidates.Add(Candidate);

		if (!IsValidCandidate(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool UDefenseComponent::TryConvertAttackToGuard(EAttackDirection GuardDirection, float GuardRequestTime, float& OutGuardDuration)
{
	OutGuardDuration = 0.0f;

	ABase* OwnerBase = Cast<ABase>(GetOwner());
	if (!OwnerBase || GuardDirection == EAttackDirection::None)
	{
		return false;
	}

	ABase* Opponent = FindPreAttackGuardOpponent();
	if (!Opponent || Opponent == OwnerBase)
	{
		return false;
	}

	AWeaponBase* OpponentWeapon = Opponent->GetCurrentWeapon();
	if (!OpponentWeapon)
	{
		return false;
	}

	const FWeaponAttackData& OpponentAttackData = OpponentWeapon->GetCurrentAttackData();
	if (!OpponentAttackData.IsValid() || !IsWeaponInActiveAttackState(OpponentWeapon->GetWeaponState()))
	{
		return false;
	}

	const float OpponentAttackStartTime = OpponentAttackData.AttackStartTime;
	const float TimeDelta = GuardRequestTime - OpponentAttackStartTime;

	if (TimeDelta <= EqualAttackTimeTolerance)
	{
		return false;
	}

	const float ValidWindow = OpponentAttackData.CounterAttackValidWindow > 0.0f ? OpponentAttackData.CounterAttackValidWindow : 0.5f;
	if (TimeDelta > ValidWindow)
	{
		return false;
	}

	const EWeaponContactDirectionRelation DirectionRelation =
		FCombatDirectionUtils::ResolveDirectionRelation(GuardDirection, OpponentAttackData.AttackDirection);

	const bool bPlayedGuard = OwnerBase->PlayCombatReactionAndGetLength(
		ECombatReactionType::Guard,
		EWeaponContactResult::Clash,
		GuardDirection,
		true,
		true,
		OutGuardDuration
	);

	if (!bPlayedGuard)
	{
		return false;
	}

	const float IncomingDamage = OpponentWeapon->GetCurrentAttackDamage();

	switch (DirectionRelation)
	{
	case EWeaponContactDirectionRelation::Opposite:
		GrantNextAttackSpeedBonus(PerfectGuardNextAttackSpeedMultiplier);
		Opponent->CancelCurrentAttackByGuard(OwnerBase, TEXT("对向格挡成功：抵消此次攻击，并给予格挡方下一次攻击速度加成"));
		break;

	case EWeaponContactDirectionRelation::NearOpposite:
		Opponent->CancelCurrentAttackByGuard(OwnerBase, TEXT("偏对向格挡成功：抵消此次攻击，无速度加成"));
		break;

	case EWeaponContactDirectionRelation::NonOpposite:
	case EWeaponContactDirectionRelation::Invalid:
	default:
		OwnerBase->ApplyDamageWithoutNonLethalHitReaction(IncomingDamage, Opponent->GetController(), OpponentWeapon);
		Opponent->CancelCurrentAttackByGuard(OwnerBase, TEXT("错误方向格挡：播放Guard，但承受完整伤害"));
		break;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][攻击前转格挡] 格挡者=%s 对手=%s 格挡方向=%s 对手方向=%s 关系=%s 时间差=%.3f 有效窗口=%.3f 伤害=%.2f Guard时长=%.3f"),
		*OwnerBase->GetName(),
		*Opponent->GetName(),
		FCombatDirectionUtils::DirectionToChinese(GuardDirection),
		FCombatDirectionUtils::DirectionToChinese(OpponentAttackData.AttackDirection),
		FCombatDirectionUtils::DirectionRelationToChinese(DirectionRelation),
		TimeDelta,
		ValidWindow,
		IncomingDamage,
		OutGuardDuration);

	return true;
}

void UDefenseComponent::GrantNextAttackSpeedBonus(float PlayRateMultiplier)
{
	if (PlayRateMultiplier <= 1.0f)
	{
		return;
	}

	NextAttackPlayRateModifier = FMath::Max(NextAttackPlayRateModifier, PlayRateMultiplier);

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][下一次攻击加速] 角色=%s 下次攻击倍率=%.3f"),
		*GetNameSafe(GetOwner()),
		NextAttackPlayRateModifier);
}

float UDefenseComponent::ConsumeNextAttackPlayRateModifier()
{
	const float Result = FMath::Max(0.1f, NextAttackPlayRateModifier);
	NextAttackPlayRateModifier = 1.0f;
	return Result;
}

bool UDefenseComponent::IsWeaponInActiveAttackState(EWeaponState State)
{
	return State == EWeaponState::Attacking || State == EWeaponState::ContactWindowOpen;
}