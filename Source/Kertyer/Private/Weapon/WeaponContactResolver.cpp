#include "Weapon/WeaponContactResolver.h"
#include "Weapon/WeaponBase.h"

namespace
{
	 constexpr float DefaultCounterAttackWindow = 0.5f;

	static float NormalizeWindow(float Window)
	{
		return Window > 0.0f ? Window : DefaultCounterAttackWindow;
	}
}

FWeaponContactResolveOutput UWeaponContactResolver::ResolveWeaponContactDetailed(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB)
{
	if (!WeaponA || !WeaponB)
	{
		FWeaponContactResolveOutput Output;
		Output.Result = EWeaponContactResult::Ignore;
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;
		return Output;
	}

	FWeaponContactResolveInput Input;
	Input.WeaponA = const_cast<AWeaponBase*>(WeaponA);
	Input.WeaponB = const_cast<AWeaponBase*>(WeaponB);
	Input.DirectionA = WeaponA->GetCurrentAttackDirection();
	Input.DirectionB = WeaponB->GetCurrentAttackDirection();
	Input.AttackTimeA = WeaponA->GetCurrentAttackData().AttackStartTime;
	Input.AttackTimeB = WeaponB->GetCurrentAttackData().AttackStartTime;
	Input.StateA = WeaponA->GetWeaponState();
	Input.StateB = WeaponB->GetWeaponState();
	Input.ResponseWindowA = WeaponA->GetCurrentAttackData().CounterAttackValidWindow;
	Input.ResponseWindowB = WeaponB->GetCurrentAttackData().CounterAttackValidWindow;

	// 兼容旧字段，但本次不参与判定。
	Input.WeightA = WeaponA->GetWeaponWeight();
	Input.WeightB = WeaponB->GetWeaponWeight();
	Input.ContactStrengthA = WeaponA->GetContactStrength();
	Input.ContactStrengthB = WeaponB->GetContactStrength();

	return ResolveWeaponContactDetailedFromInput(Input);
}

FWeaponContactResolveOutput UWeaponContactResolver::ResolveWeaponContactDetailedFromInput(const FWeaponContactResolveInput& Input)
{
	FWeaponContactResolveOutput Output;

	Output.SlowerSide = GetSlowerSide(Input.AttackTimeA, Input.AttackTimeB);
	Output.FasterSide = GetFasterSide(Input.AttackTimeA, Input.AttackTimeB);
	Output.TimeDelta = FMath::Abs(Input.AttackTimeA - Input.AttackTimeB);
	Output.ValidResponseWindow = GetValidResponseWindow(Input, Output.FasterSide);
	Output.bIsValidTimedResponse = Output.TimeDelta <= Output.ValidResponseWindow;

	const bool bAActive = IsActiveWeaponState(Input.StateA);
	const bool bBActive = IsActiveWeaponState(Input.StateB);

	if (!Input.WeaponA || !Input.WeaponB)
	{
		Output.Result = EWeaponContactResult::Ignore;
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;
		return Output;
	}

	if (!bAActive && !bBActive)
	{
		Output.Result = EWeaponContactResult::Ignore;
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;
		return Output;
	}

	if (!bAActive || !bBActive)
	{
		Output.Result = EWeaponContactResult::Hit;
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;
		Output.DamagedSide = bAActive ? EWeaponContactSide::WeaponB : EWeaponContactSide::WeaponA;
		Output.AdvantageSide = bAActive ? EWeaponContactSide::WeaponA : EWeaponContactSide::WeaponB;
		Output.bShouldDamageSlowerBody = true;
		return Output;
	}

	if (Input.DirectionA == EAttackDirection::None || Input.DirectionB == EAttackDirection::None)
	{
		Output.Result = EWeaponContactResult::Hit;
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;
		Output.DamagedSide = Output.SlowerSide;
		Output.AdvantageSide = Output.FasterSide;
		Output.bShouldDamageSlowerBody = Output.DamagedSide != EWeaponContactSide::None && Output.DamagedSide != EWeaponContactSide::Both;
		return Output;
	}

	if (IsOppositeDirection(Input.DirectionA, Input.DirectionB))
	{
		Output.DirectionRelation = EWeaponContactDirectionRelation::Opposite;
	}
	else if (IsNearOppositeDirection(Input.DirectionA, Input.DirectionB))
	{
		Output.DirectionRelation = EWeaponContactDirectionRelation::NearOpposite;
	}
	else
	{
		Output.DirectionRelation = EWeaponContactDirectionRelation::NonOpposite;
	}

	// 后发方超过先发方响应窗口：视为响应无效，较慢方直接受到伤害。
	if (!Output.bIsValidTimedResponse)
	{
		Output.Result = EWeaponContactResult::Hit;
		Output.DamagedSide = Output.SlowerSide;
		Output.AdvantageSide = Output.FasterSide;
		Output.bShouldDamageSlowerBody = Output.DamagedSide != EWeaponContactSide::None && Output.DamagedSide != EWeaponContactSide::Both;
		return Output;
	}

	// 对向攻击：较慢发起方获得优势；暂不做优势逻辑，只打印日志，反馈统一走 Clash。
	if (Output.DirectionRelation == EWeaponContactDirectionRelation::Opposite)
	{
		Output.Result = EWeaponContactResult::Clash;
		Output.AdvantageSide = Output.SlowerSide;
		Output.DamagedSide = EWeaponContactSide::None;
		Output.bShouldDamageSlowerBody = false;
		return Output;
	}

	// 较对向攻击：双方平衡，保持武器反馈，不造成身体伤害。
	if (Output.DirectionRelation == EWeaponContactDirectionRelation::NearOpposite)
	{
		Output.Result = EWeaponContactResult::Clash;
		Output.AdvantageSide = EWeaponContactSide::Both;
		Output.DamagedSide = EWeaponContactSide::None;
		Output.bShouldDamageSlowerBody = false;
		return Output;
	}

	// 非对向攻击：较慢方直接受到伤害。
	Output.Result = EWeaponContactResult::Hit;
	Output.AdvantageSide = Output.FasterSide;
	Output.DamagedSide = Output.SlowerSide;
	Output.bShouldDamageSlowerBody = Output.DamagedSide != EWeaponContactSide::None && Output.DamagedSide != EWeaponContactSide::Both;
	return Output;
}

EWeaponContactResult UWeaponContactResolver::ResolveWeaponContact(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB)
{
	return ResolveWeaponContactDetailed(WeaponA, WeaponB).Result;
}

EWeaponContactResult UWeaponContactResolver::ResolveWeaponContactFromInput(const FWeaponContactResolveInput& Input)
{
	return ResolveWeaponContactDetailedFromInput(Input).Result;
}

bool UWeaponContactResolver::IsOppositeDirection(EAttackDirection A, EAttackDirection B)
{
	return GetCircularDirectionDelta(A, B) == 4;
}

bool UWeaponContactResolver::IsNearOppositeDirection(EAttackDirection A, EAttackDirection B)
{
	// 八方向下，差 3 或 5 都属于“较对向”，用环形差值后统一为 3。
	return GetCircularDirectionDelta(A, B) == 3;
}

bool UWeaponContactResolver::IsActiveWeaponState(EWeaponState State)
{
	return State == EWeaponState::Attacking || State == EWeaponState::ContactWindowOpen;
}

int32 UWeaponContactResolver::DirectionToIndex(EAttackDirection Direction)
{
	switch (Direction)
	{
	case EAttackDirection::Up:
		return 0;
	case EAttackDirection::UpRight:
		return 1;
	case EAttackDirection::Right:
		return 2;
	case EAttackDirection::DownRight:
		return 3;
	case EAttackDirection::Down:
		return 4;
	case EAttackDirection::DownLeft:
		return 5;
	case EAttackDirection::Left:
		return 6;
	case EAttackDirection::UpLeft:
		return 7;
	default:
		return INDEX_NONE;
	}
}

int32 UWeaponContactResolver::GetCircularDirectionDelta(EAttackDirection A, EAttackDirection B)
{
	const int32 IndexA = DirectionToIndex(A);
	const int32 IndexB = DirectionToIndex(B);

	if (IndexA == INDEX_NONE || IndexB == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 RawDelta = FMath::Abs(IndexA - IndexB);
	return FMath::Min(RawDelta, 8 - RawDelta);
}

EWeaponContactSide UWeaponContactResolver::GetSlowerSide(float AttackTimeA, float AttackTimeB)
{
	if (AttackTimeA < 0.0f && AttackTimeB < 0.0f)
	{
		return EWeaponContactSide::None;
	}

	if (AttackTimeA < 0.0f)
	{
		return EWeaponContactSide::WeaponA;
	}

	if (AttackTimeB < 0.0f)
	{
		return EWeaponContactSide::WeaponB;
	}

	if (FMath::IsNearlyEqual(AttackTimeA, AttackTimeB, KINDA_SMALL_NUMBER))
	{
		return EWeaponContactSide::Both;
	}

	return AttackTimeA > AttackTimeB ? EWeaponContactSide::WeaponA : EWeaponContactSide::WeaponB;
}

EWeaponContactSide UWeaponContactResolver::GetFasterSide(float AttackTimeA, float AttackTimeB)
{
	if (AttackTimeA < 0.0f && AttackTimeB < 0.0f)
	{
		return EWeaponContactSide::None;
	}

	if (AttackTimeA < 0.0f)
	{
		return EWeaponContactSide::WeaponB;
	}

	if (AttackTimeB < 0.0f)
	{
		return EWeaponContactSide::WeaponA;
	}

	if (FMath::IsNearlyEqual(AttackTimeA, AttackTimeB, KINDA_SMALL_NUMBER))
	{
		return EWeaponContactSide::Both;
	}

	return AttackTimeA < AttackTimeB ? EWeaponContactSide::WeaponA : EWeaponContactSide::WeaponB;
}

float UWeaponContactResolver::GetValidResponseWindow(const FWeaponContactResolveInput& Input, EWeaponContactSide FasterSide)
{
	if (FasterSide == EWeaponContactSide::WeaponA)
	{
		return NormalizeWindow(Input.ResponseWindowA);
	}

	if (FasterSide == EWeaponContactSide::WeaponB)
	{
		return NormalizeWindow(Input.ResponseWindowB);
	}

	return NormalizeWindow(Input.ResponseWindowA);
}