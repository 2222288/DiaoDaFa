// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/StaticMeshComponent.h"
#include "Base.generated.h"


UCLASS()
class KERTYER_API ABase : public ACharacter
{
    GENERATED_BODY()

public:

    ABase();

    // 武器静态网格体
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    TObjectPtr<class UStaticMeshComponent> WeaponMesh;

    // 最大血量 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float MaxHealth;

    // 当前血量 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float CurrentHealth;

    // 治疗函数 
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void Treat(float HealAmount);

    // 开启武器伤害碰撞
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void EnableWeaponDamage();

    // 关闭武器伤害碰撞
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void DisableWeaponDamage();

	// 武器碰撞事件处理函数
	//@param OverlappedComponent - 发生重叠的组件（通常是武器）
	//@param OtherActor - 另一个参与重叠的Actor（可能是敌人）
	//@param OtherComp - 另一个参与重叠的组件（敌人的碰撞体）
	//@param OtherBodyIndex - 另一个组件的物理身体索引（通常不重要）
	//@param bFromSweep - 是否是从Sweep（移动）引起的重叠
	//@param SweepResult - 包含重叠详细信息的结构体（比如碰撞点、法线等）
    UFUNCTION()
    void OnWeaponOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    // 受伤逻辑
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
    virtual void BeginPlay() override;

    // 本次攻击已经命中过的角色
    UPROPERTY()
    TSet<TObjectPtr<AActor>> HitActors;

    bool bWeaponDamageEnabled = false;
};