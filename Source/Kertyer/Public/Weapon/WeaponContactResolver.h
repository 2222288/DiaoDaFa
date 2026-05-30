#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponContactResolver.generated.h"

class AWeaponBase;

/** 武器接触判定输入数据。 */
USTRUCT(BlueprintType)
struct KERTYER_API FWeaponContactResolveInput
{
	GENERATED_BODY()

	/** A 方武器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器"))
	TObjectPtr<AWeaponBase> WeaponA = nullptr;

	/** B 方武器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器"))
	TObjectPtr<AWeaponBase> WeaponB = nullptr;

	/** A 方攻击方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方攻击方向"))
	EAttackDirection DirectionA = EAttackDirection::None;

	/** B 方攻击方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方攻击方向"))
	EAttackDirection DirectionB = EAttackDirection::None;

	/** A 方攻击开始时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方攻击开始时间"))
	float AttackTimeA = -1.0f;

	/** B 方攻击开始时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方攻击开始时间"))
	float AttackTimeB = -1.0f;

	/** A 方武器状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器状态"))
	EWeaponState StateA = EWeaponState::Idle;

	/** B 方武器状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器状态"))
	EWeaponState StateB = EWeaponState::Idle;

	/** A 方武器重量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器重量"))
	float WeightA = 1.0f;

	/** B 方武器重量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器重量"))
	float WeightB = 1.0f;

	/** A 方接触强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方接触强度"))
	float ContactStrengthA = 1.0f;

	/** B 方接触强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方接触强度"))
	float ContactStrengthB = 1.0f;
};

/**
 * 武器接触判定工具：
 * 根据双方方向、状态、攻击时间、重量和接触强度，计算武器碰撞结果。
 */
UCLASS()
class KERTYER_API UWeaponContactResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 根据手动输入数据解析武器接触结果。 */
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "解析武器接触结果"))
	static EWeaponContactResult ResolveWeaponContactFromInput(const FWeaponContactResolveInput& Input);

	/** 根据两个武器对象解析武器接触结果。 */
	static EWeaponContactResult ResolveWeaponContact(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB);

private:
	/** 判断两个方向是否相反。 */
	static bool IsOppositeDirection(EAttackDirection A, EAttackDirection B);

	/** 判断武器状态是否能参与接触判定。 */
	static bool IsActiveWeaponState(EWeaponState State);

	/** 把攻击方向转换为八方向索引。 */
	static int32 DirectionToIndex(EAttackDirection Direction);
};