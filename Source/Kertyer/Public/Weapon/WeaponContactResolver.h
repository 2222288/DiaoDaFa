#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponContactResolver.generated.h"

class AWeaponBase;

USTRUCT(BlueprintType)
struct KERTYER_API FWeaponContactResolveInput
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    TObjectPtr<AWeaponBase> WeaponA = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    TObjectPtr<AWeaponBase> WeaponB = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EAttackDirection DirectionA = EAttackDirection::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EAttackDirection DirectionB = EAttackDirection::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float AttackTimeA = -1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float AttackTimeB = -1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EWeaponState StateA = EWeaponState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EWeaponState StateB = EWeaponState::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float WeightA = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float WeightB = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float ContactStrengthA = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float ContactStrengthB = 1.0f;
};

UCLASS()
class KERTYER_API UWeaponContactResolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    static EWeaponContactResult ResolveWeaponContactFromInput(const FWeaponContactResolveInput& Input);

    static EWeaponContactResult ResolveWeaponContact(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB);

private:
    static bool IsOppositeDirection(EAttackDirection A, EAttackDirection B);
    static bool IsActiveWeaponState(EWeaponState State);
    static int32 DirectionToIndex(EAttackDirection Direction);
};