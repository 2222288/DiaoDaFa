#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Animation/AnimMontage.h"
#include "AttackDH.generated.h"

UENUM(BlueprintType)
enum class EAttackDirection : uint8
{
    None,
    Up,
    UpRight,
    Right,
    DownRight,
    Down,
    DownLeft,
    Left,
    UpLeft
};

UENUM(BlueprintType)
enum class EAttackState : uint8
{
    Idle,
    Sampling,
    AttackingLocked,
    ComboWindowOpen,
    SamplingLocked,
    SamplingComboWindow 
};


USTRUCT(BlueprintType)
struct FAttack : public FTableRowBase
{
    GENERATED_BODY()

    // 名字
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    FName AttackID;

    // 攻击方向
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    EAttackDirection AttackDirection = EAttackDirection::None;

    // 播放的动画
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    TObjectPtr<UAnimMontage> AttackMontage = nullptr;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    FName MontageSection = NAME_None;

    // 伤害
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    float Damage = 50.f;

    // 窗口期
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    float WindowTime = 0.25f;

    //段数
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    int32 Level = 0;

};

UCLASS(BlueprintType)
class KERTYER_API AAttackDH : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
    TObjectPtr<UDataTable> AttackData = nullptr;
};
