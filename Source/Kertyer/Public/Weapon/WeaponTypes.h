#pragma once

#include "CoreMinimal.h"
#include "DataAsset/AttackDH.h"
#include "WeaponTypes.generated.h"


UENUM(BlueprintType)
//武器类型
enum class EWeaponType : uint8
{
    None UMETA(DisplayName = "None"),
    Sword UMETA(DisplayName = "Sword"),
    Axe UMETA(DisplayName = "Axe"),
    Spear UMETA(DisplayName = "Spear")
};

UENUM(BlueprintType)
//武器状态
enum class EWeaponState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Attacking UMETA(DisplayName = "Attacking"),
    ContactWindowOpen UMETA(DisplayName = "Contact Window Open"),
    Recovering UMETA(DisplayName = "Recovering"),
    Disabled UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
//武器接触结果
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

	//攻击方向
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EAttackDirection AttackDirection = EAttackDirection::None;

	//攻击开始时间
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float AttackStartTime = -1.0f;

	//攻击类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    FName AttackType = NAME_None;

	//基础伤害
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float BaseDamage = 0.0f;

	//伤害修正
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float DamageModifier = 1.0f;

	//检查攻击数据是否有效
    bool IsValid() const
    {
        return AttackDirection != EAttackDirection::None && AttackStartTime >= 0.0f;
    }

	//计算最终伤害
    float GetFinalDamage() const
    {
        return FMath::Max(0.0f, BaseDamage * DamageModifier);
    }
};