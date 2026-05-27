#pragma once

#include "CoreMinimal.h"
#include "DataAsset/AttackDH.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    None UMETA(DisplayName = "None"),
    Sword UMETA(DisplayName = "Sword"),
    Axe UMETA(DisplayName = "Axe"),
    Spear UMETA(DisplayName = "Spear")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Attacking UMETA(DisplayName = "Attacking"),
    ContactWindowOpen UMETA(DisplayName = "Contact Window Open"),
    Recovering UMETA(DisplayName = "Recovering"),
    Disabled UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class EWeaponContactResult : uint8
{
    Clash UMETA(DisplayName = "Clash"),
    Deflect UMETA(DisplayName = "Deflect"),
    Interrupt UMETA(DisplayName = "Interrupt"),
    Hit UMETA(DisplayName = "Hit"),
    Ignore UMETA(DisplayName = "Ignore")
};

USTRUCT(BlueprintType)
struct KERTYER_API FWeaponAttackData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EAttackDirection AttackDirection = EAttackDirection::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float AttackStartTime = -1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    FName AttackType = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float BaseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float DamageModifier = 1.0f;

    bool IsValid() const
    {
        return AttackDirection != EAttackDirection::None && AttackStartTime >= 0.0f;
    }

    float GetFinalDamage() const
    {
        return FMath::Max(0.0f, BaseDamage * DamageModifier);
    }
};