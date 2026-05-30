#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Animation/AnimMontage.h"
#include "AttackDH.generated.h"

/** 八方向攻击方向。 */
UENUM(BlueprintType)
enum class EAttackDirection : uint8
{
	None UMETA(DisplayName = "无"),
	Up UMETA(DisplayName = "上"),
	UpRight UMETA(DisplayName = "右上"),
	Right UMETA(DisplayName = "右"),
	DownRight UMETA(DisplayName = "右下"),
	Down UMETA(DisplayName = "下"),
	DownLeft UMETA(DisplayName = "左下"),
	Left UMETA(DisplayName = "左"),
	UpLeft UMETA(DisplayName = "左上")
};

/** 攻击状态机状态。 */
UENUM(BlueprintType)
enum class EAttackState : uint8
{
	Idle UMETA(DisplayName = "空闲"),
	Sampling UMETA(DisplayName = "采样中"),
	AttackingLocked UMETA(DisplayName = "攻击锁定"),
	ComboWindowOpen UMETA(DisplayName = "连击窗口打开"),
	SamplingLocked UMETA(DisplayName = "采样但锁定"),
	SamplingComboWindow UMETA(DisplayName = "采样且连击窗口打开")
};

/** 攻击数据表中的一行攻击配置。 */
USTRUCT(BlueprintType)
struct FAttack : public FTableRowBase
{
	GENERATED_BODY()

	/** 攻击 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击ID"))
	FName AttackID;

	/** 攻击方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击方向"))
	EAttackDirection AttackDirection = EAttackDirection::None;

	/** 播放的攻击动画 Montage。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击动画Montage"))
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** Montage 中要跳转播放的 Section 名称。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "动画段名称"))
	FName MontageSection = NAME_None;

	/** 攻击基础伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "基础伤害"))
	float Damage = 50.f;

	/** 连击窗口时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "连击窗口时间"))
	float WindowTime = 0.25f;

	/** 攻击段数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack", meta = (DisplayName = "攻击段数"))
	int32 Level = 0;
};

/** 攻击数据资产持有类。 */
UCLASS(BlueprintType)
class KERTYER_API AAttackDH : public AActor
{
	GENERATED_BODY()

public:
	/** 攻击数据表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (DisplayName = "攻击数据表"))
	TObjectPtr<UDataTable> AttackData = nullptr;
};