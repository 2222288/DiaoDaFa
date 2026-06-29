#include "Weapon/WeaponContactResolver.h"
#include "Weapon/WeaponBase.h"
#include "Combat/CombatDirectionUtils.h"

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

	Output.EqualTimingTolerance = NormalizeEqualAttackTimeTolerance(Input.EqualAttackTimeTolerance);
	Output.TimingRelation = GetTimingRelation(Input.AttackTimeA, Input.AttackTimeB, Output.EqualTimingTolerance);
	Output.SlowerSide = GetSlowerSide(Input.AttackTimeA, Input.AttackTimeB, Output.EqualTimingTolerance);
	Output.FasterSide = GetFasterSide(Input.AttackTimeA, Input.AttackTimeB, Output.EqualTimingTolerance);
	Output.TimeDelta = FMath::Abs(Input.AttackTimeA - Input.AttackTimeB);
	Output.ValidResponseWindow = GetValidResponseWindow(Input, Output.FasterSide);
	Output.bIsEqualTiming = Output.TimingRelation == EAttackTimingRelation::Equal;
	Output.bIsValidTimedResponse = Output.bIsEqualTiming || Output.TimeDelta <= Output.ValidResponseWindow;

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
		Output.DirectionRelation = EWeaponContactDirectionRelation::Invalid;

		if (Output.bIsEqualTiming)
		{
			Output.Result = EWeaponContactResult::Deflect;
			Output.AdvantageSide = EWeaponContactSide::Both;
			Output.DamagedSide = EWeaponContactSide::None;
			Output.bShouldDamageSlowerBody = false;
			return Output;
		}

		Output.Result = EWeaponContactResult::Hit;
		Output.DamagedSide = Output.SlowerSide;
		Output.AdvantageSide = Output.FasterSide;
		Output.bShouldDamageSlowerBody =
			Output.DamagedSide != EWeaponContactSide::None &&
			Output.DamagedSide != EWeaponContactSide::Both;
		return Output;
	}

	Output.DirectionRelation = FCombatDirectionUtils::ResolveDirectionRelation(Input.DirectionA, Input.DirectionB);

	// 速度一致：双方攻击动画继续；一旦武器碰撞，双方播放弹开。
	if (Output.bIsEqualTiming)
	{
		Output.Result = EWeaponContactResult::Deflect;
		Output.AdvantageSide = EWeaponContactSide::Both;
		Output.DamagedSide = EWeaponContactSide::None;
		Output.bShouldDamageSlowerBody = false;
		return Output;
	}

	// 后发超过响应窗口：较慢方受伤。
	if (!Output.bIsValidTimedResponse)
	{
		Output.Result = EWeaponContactResult::Hit;
		Output.DamagedSide = Output.SlowerSide;
		Output.AdvantageSide = Output.FasterSide;
		Output.bShouldDamageSlowerBody =
			Output.DamagedSide != EWeaponContactSide::None &&
			Output.DamagedSide != EWeaponContactSide::Both;
		return Output;
	}

	// 只要后发方慢于对方且仍在有效响应窗口内，就视为进入格挡。
	// 方向只决定此次攻击效果，不决定是否触发 Guard。
	if (Output.DirectionRelation == EWeaponContactDirectionRelation::Opposite)
	{
		Output.Result = EWeaponContactResult::Clash;
		Output.AdvantageSide = Output.SlowerSide;
		Output.DamagedSide = EWeaponContactSide::None;
		Output.bShouldDamageSlowerBody = false;
		return Output;
	}

	if (Output.DirectionRelation == EWeaponContactDirectionRelation::NearOpposite)
	{
		Output.Result = EWeaponContactResult::Clash;
		Output.AdvantageSide = Output.SlowerSide;
		Output.DamagedSide = EWeaponContactSide::None;
		Output.bShouldDamageSlowerBody = false;
		return Output;
	}

	// 非对向：仍然是格挡动作，但效果失败，较慢方吃满伤害。
	Output.Result = EWeaponContactResult::Clash;
	Output.AdvantageSide = Output.FasterSide;
	Output.DamagedSide = Output.SlowerSide;
	Output.bShouldDamageSlowerBody =
		Output.DamagedSide != EWeaponContactSide::None &&
		Output.DamagedSide != EWeaponContactSide::Both;

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
	return FCombatDirectionUtils::CircularDirectionDelta(A, B) == 4;
}

bool UWeaponContactResolver::IsNearOppositeDirection(EAttackDirection A, EAttackDirection B)
{
	// 八方向下，差 3 或 5 都属于“较对向”，用环形差值后统一为 3。
	return FCombatDirectionUtils::CircularDirectionDelta(A, B) == 3;
}

bool UWeaponContactResolver::IsActiveWeaponState(EWeaponState State)
{
	return State == EWeaponState::Attacking || State == EWeaponState::ContactWindowOpen;
}

int32 UWeaponContactResolver::DirectionToIndex(EAttackDirection Direction)
{
	return FCombatDirectionUtils::DirectionToIndex(Direction);
}

int32 UWeaponContactResolver::GetCircularDirectionDelta(EAttackDirection A, EAttackDirection B)
{
	return FCombatDirectionUtils::CircularDirectionDelta(A, B);
}

EAttackTimingRelation UWeaponContactResolver::GetTimingRelation(
	float AttackTimeA,
	float AttackTimeB,
	float EqualAttackTimeTolerance
)
{
	const float Tolerance = NormalizeEqualAttackTimeTolerance(EqualAttackTimeTolerance);

	if (AttackTimeA < 0.0f || AttackTimeB < 0.0f)
	{
		return EAttackTimingRelation::Invalid;
	}

	if (FMath::Abs(AttackTimeA - AttackTimeB) <= Tolerance)
	{
		return EAttackTimingRelation::Equal;
	}

	return AttackTimeA < AttackTimeB
		? EAttackTimingRelation::AIsFaster
		: EAttackTimingRelation::BIsFaster;
}

EWeaponContactSide UWeaponContactResolver::GetSlowerSide(
	float AttackTimeA,
	float AttackTimeB,
	float EqualAttackTimeTolerance
)
{
	const EAttackTimingRelation TimingRelation =
		GetTimingRelation(AttackTimeA, AttackTimeB, EqualAttackTimeTolerance);

	switch (TimingRelation)
	{
	case EAttackTimingRelation::AIsFaster:
		return EWeaponContactSide::WeaponB;

	case EAttackTimingRelation::BIsFaster:
		return EWeaponContactSide::WeaponA;

	case EAttackTimingRelation::Equal:
		return EWeaponContactSide::Both;

	case EAttackTimingRelation::Invalid:
	default:
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

		return EWeaponContactSide::None;
	}
}

EWeaponContactSide UWeaponContactResolver::GetFasterSide(
	float AttackTimeA,
	float AttackTimeB,
	float EqualAttackTimeTolerance
)
{
	const EAttackTimingRelation TimingRelation =
		GetTimingRelation(AttackTimeA, AttackTimeB, EqualAttackTimeTolerance);

	switch (TimingRelation)
	{
	case EAttackTimingRelation::AIsFaster:
		return EWeaponContactSide::WeaponA;

	case EAttackTimingRelation::BIsFaster:
		return EWeaponContactSide::WeaponB;

	case EAttackTimingRelation::Equal:
		return EWeaponContactSide::Both;

	case EAttackTimingRelation::Invalid:
	default:
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

		return EWeaponContactSide::None;
	}
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

float UWeaponContactResolver::NormalizeEqualAttackTimeTolerance(float EqualAttackTimeTolerance)
{
	return EqualAttackTimeTolerance > 0.0f ? EqualAttackTimeTolerance : 0.12f;
}