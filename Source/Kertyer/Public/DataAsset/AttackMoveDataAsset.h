#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Combat/CombatTypes.h"
#include "AttackMoveDataAsset.generated.h"

/**
 * 单个主动攻击动作配置。
 *
 * 只描述主动出招。
 * 不负责格挡。
 * 不负责受击。
 * 不负责双方出刀时机判断。
 */
USTRUCT(BlueprintType)
struct KERTYER_API FAttackMoveData
{
	GENERATED_BODY()

	/** 攻击 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击ID"))
	FName AttackID = NAME_None;

	/** 攻击方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击方向"))
	EAttackDirection AttackDirection = EAttackDirection::None;

	/** 主动攻击动画 Montage。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击动画Montage"))
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** Montage Section。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "动画段名称"))
	FName MontageSection = NAME_None;

	/** 基础伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "基础伤害"))
	float Damage = 50.0f;

	/** 连击窗口时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "连击窗口时间"))
	float WindowTime = 0.25f;

	/** 攻击段数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击段数"))
	int32 Level = 0;

	/**
	 * 本次攻击给后发方的有效响应窗口。
	 *
	 * 例如：
	 * 先发方攻击后，后发方在这个时间内出对向攻击，可以判定为格挡。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击响应有效窗口"))
	float CounterAttackValidWindow = 0.5f;
};

/**
 * 一套主动攻击动作数据。
 *
 * 玩家和敌人都可以持有一个这个资产。
 * 玩家按方向查找攻击。
 * 敌人可以随机取一条攻击。
 */
UCLASS(BlueprintType)
class KERTYER_API UAttackMoveDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击动作列表"))
	TArray<FAttackMoveData> AttackMoves;

	const FAttackMoveData* FindAttackByDirection(EAttackDirection Direction) const;

	const FAttackMoveData* GetRandomAttack() const;
};