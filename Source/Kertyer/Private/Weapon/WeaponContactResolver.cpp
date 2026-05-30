#include "Weapon/WeaponContactResolver.h"
#include "Weapon/WeaponBase.h"

EWeaponContactResult UWeaponContactResolver::ResolveWeaponContact(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB)
{
    if (!WeaponA || !WeaponB)
    {
        return EWeaponContactResult::Ignore;
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
    Input.WeightA = WeaponA->GetWeaponWeight();
    Input.WeightB = WeaponB->GetWeaponWeight();
    Input.ContactStrengthA = WeaponA->GetContactStrength();
    Input.ContactStrengthB = WeaponB->GetContactStrength();

    return ResolveWeaponContactFromInput(Input);
}

EWeaponContactResult UWeaponContactResolver::ResolveWeaponContactFromInput(const FWeaponContactResolveInput& Input)
{
    if (!Input.WeaponA || !Input.WeaponB)
    {
        return EWeaponContactResult::Ignore;
    }

    if (!IsActiveWeaponState(Input.StateA) || !IsActiveWeaponState(Input.StateB))
    {
        return EWeaponContactResult::Hit;
    }

    if (Input.DirectionA == EAttackDirection::None || Input.DirectionB == EAttackDirection::None)
    {
        return EWeaponContactResult::Hit;
    }

    if (!IsOppositeDirection(Input.DirectionA, Input.DirectionB))
    {
        return EWeaponContactResult::Hit;
    }

    const float TimeDelta = FMath::Abs(Input.AttackTimeA - Input.AttackTimeB);
    const float AverageContact = (Input.ContactStrengthA + Input.ContactStrengthB) * 0.5f;
    const float AverageWeight = (Input.WeightA + Input.WeightB) * 0.5f;

    const float SimultaneousThreshold = 0.12f + FMath::Clamp(AverageContact * 0.01f, 0.0f, 0.04f);
    const float DeflectThreshold = 0.32f + FMath::Clamp(AverageWeight * 0.01f, 0.0f, 0.08f);

    if (TimeDelta <= SimultaneousThreshold)
    {
        return EWeaponContactResult::Clash;
    }

    if (TimeDelta <= DeflectThreshold)
    {
        return EWeaponContactResult::Deflect;
    }

    return EWeaponContactResult::Interrupt;
}

bool UWeaponContactResolver::IsOppositeDirection(EAttackDirection A, EAttackDirection B)
{
    const int32 IndexA = DirectionToIndex(A);
    const int32 IndexB = DirectionToIndex(B);

    if (IndexA < 0 || IndexB < 0)
    {
        return false;
    }

    return ((IndexA + 4) % 8) == IndexB;
}

bool UWeaponContactResolver::IsActiveWeaponState(EWeaponState State)
{
    return State == EWeaponState::Attacking || State == EWeaponState::ContactWindowOpen;
}

int32 UWeaponContactResolver::DirectionToIndex(EAttackDirection Direction)
{
    switch (Direction)
    {
    case EAttackDirection::Up: return 0;
    case EAttackDirection::UpRight: return 1;
    case EAttackDirection::Right: return 2;
    case EAttackDirection::DownRight: return 3;
    case EAttackDirection::Down: return 4;
    case EAttackDirection::DownLeft: return 5;
    case EAttackDirection::Left: return 6;
    case EAttackDirection::UpLeft: return 7;
    default: return -1;
    }
}