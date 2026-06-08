#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None UMETA(DisplayName = "无"),
	Sword UMETA(DisplayName = "剑"),
	Axe UMETA(DisplayName = "斧"),
	Spear UMETA(DisplayName = "长矛")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle UMETA(DisplayName = "空闲"),
	Attacking UMETA(DisplayName = "攻击中"),
	ContactWindowOpen UMETA(DisplayName = "接触窗口打开"),
	Recovering UMETA(DisplayName = "恢复中"),
	Disabled UMETA(DisplayName = "失效")
};

UENUM(BlueprintType)
enum class EWeaponContactResult : uint8
{
	Clash UMETA(DisplayName = "武器反馈"),
	Deflect UMETA(DisplayName = "偏斜"),
	Interrupt UMETA(DisplayName = "打断"),
	Hit UMETA(DisplayName = "造成伤害"),
	Ignore UMETA(DisplayName = "忽略")
};

UENUM(BlueprintType)
enum class EWeaponContactSide : uint8
{
	None UMETA(DisplayName = "无"),
	WeaponA UMETA(DisplayName = "A方"),
	WeaponB UMETA(DisplayName = "B方"),
	Both UMETA(DisplayName = "双方")
};

UENUM(BlueprintType)
enum class EWeaponContactDirectionRelation : uint8
{
	Invalid UMETA(DisplayName = "无效"),
	Opposite UMETA(DisplayName = "对向攻击"),
	NearOpposite UMETA(DisplayName = "较对向攻击"),
	NonOpposite UMETA(DisplayName = "非对向攻击")
};

UENUM(BlueprintType)
enum class EAttackTimingRelation : uint8
{
	Invalid UMETA(DisplayName = "无效"),
	AIsFaster UMETA(DisplayName = "A方较快"),
	BIsFaster UMETA(DisplayName = "B方较快"),
	Equal UMETA(DisplayName = "双方速度一致")
};

USTRUCT(BlueprintType)
struct KERTYER_API FWeaponAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "攻击方向"))
	EAttackDirection AttackDirection = EAttackDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "攻击开始时间"))
	float AttackStartTime = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "攻击ID"))
	FName AttackType = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "基础伤害"))
	float BaseDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "伤害倍率"))
	float DamageModifier = 1.0f;

	// 先发攻击方给后发攻击方的有效响应时间。
	// 现在按您的要求暂定 0.5 秒；后续可以改成由先发方动画时长传入。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "攻击响应有效窗口"))
	float CounterAttackValidWindow = 0.5f;

	bool IsValid() const
	{
		return AttackDirection != EAttackDirection::None && AttackStartTime >= 0.0f;
	}

	float GetFinalDamage() const
	{
		return FMath::Max(0.0f, BaseDamage * DamageModifier);
	}
};

USTRUCT(BlueprintType)
struct KERTYER_API FWeaponContactResolveOutput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "武器接触结果"))
	EWeaponContactResult Result = EWeaponContactResult::Ignore;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "方向关系"))
	EWeaponContactDirectionRelation DirectionRelation = EWeaponContactDirectionRelation::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "攻击时机关系"))
	EAttackTimingRelation TimingRelation = EAttackTimingRelation::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "较慢方"))
	EWeaponContactSide SlowerSide = EWeaponContactSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "较快方"))
	EWeaponContactSide FasterSide = EWeaponContactSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "优势方"))
	EWeaponContactSide AdvantageSide = EWeaponContactSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "受伤方"))
	EWeaponContactSide DamagedSide = EWeaponContactSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "双方发起时间差"))
	float TimeDelta = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "有效响应窗口"))
	float ValidResponseWindow = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "等速判定容差"))
	float EqualTimingTolerance = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "是否有效响应"))
	bool bIsValidTimedResponse = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "是否速度一致"))
	bool bIsEqualTiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Weapon", meta = (DisplayName = "是否伤害较慢方身体"))
	bool bShouldDamageSlowerBody = false;
};