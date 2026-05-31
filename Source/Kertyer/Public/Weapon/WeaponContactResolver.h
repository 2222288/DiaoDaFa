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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器"))
	TObjectPtr<AWeaponBase> WeaponA = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器"))
	TObjectPtr<AWeaponBase> WeaponB = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方攻击方向"))
	EAttackDirection DirectionA = EAttackDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方攻击方向"))
	EAttackDirection DirectionB = EAttackDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方攻击开始时间"))
	float AttackTimeA = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方攻击开始时间"))
	float AttackTimeB = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器状态"))
	EWeaponState StateA = EWeaponState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器状态"))
	EWeaponState StateB = EWeaponState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方响应有效窗口"))
	float ResponseWindowA = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方响应有效窗口"))
	float ResponseWindowB = 0.5f;

	// 保留字段，避免旧蓝图或旧代码断引用；本次判定不再使用重量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方武器重量"))
	float WeightA = 1.0f;

	// 保留字段，避免旧蓝图或旧代码断引用；本次判定不再使用重量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方武器重量"))
	float WeightB = 1.0f;

	// 保留字段，避免旧蓝图或旧代码断引用；本次判定不再使用接触强度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "A方接触强度"))
	float ContactStrengthA = 1.0f;

	// 保留字段，避免旧蓝图或旧代码断引用；本次判定不再使用接触强度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (DisplayName = "B方接触强度"))
	float ContactStrengthB = 1.0f;
};

UCLASS()
class KERTYER_API UWeaponContactResolver : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "解析武器接触详细结果"))
	static FWeaponContactResolveOutput ResolveWeaponContactDetailedFromInput(const FWeaponContactResolveInput& Input);

	static FWeaponContactResolveOutput ResolveWeaponContactDetailed(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB);

	UFUNCTION(BlueprintPure, Category = "Combat|Weapon", meta = (DisplayName = "解析武器接触结果"))
	static EWeaponContactResult ResolveWeaponContactFromInput(const FWeaponContactResolveInput& Input);

	static EWeaponContactResult ResolveWeaponContact(const AWeaponBase* WeaponA, const AWeaponBase* WeaponB);

private:
	static bool IsOppositeDirection(EAttackDirection A, EAttackDirection B);
	static bool IsNearOppositeDirection(EAttackDirection A, EAttackDirection B);
	static bool IsActiveWeaponState(EWeaponState State);
	static int32 DirectionToIndex(EAttackDirection Direction);
	static int32 GetCircularDirectionDelta(EAttackDirection A, EAttackDirection B);

	static EWeaponContactSide GetSlowerSide(float AttackTimeA, float AttackTimeB);
	static EWeaponContactSide GetFasterSide(float AttackTimeA, float AttackTimeB);
	static float GetValidResponseWindow(const FWeaponContactResolveInput& Input, EWeaponContactSide FasterSide);
};