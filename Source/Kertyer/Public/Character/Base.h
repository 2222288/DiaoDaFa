// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
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

    // 最大血量 (可以在蓝图里配，比如Boss血厚一点)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float MaxHealth;

    // 当前血量 (通常只读，看调试用)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float CurrentHealth;

    // 治疗函数 (加血)
    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void Treat(float HealAmount);

    // 重写引擎自带的“受伤”函数
    // 只要有任何东西对这个角色造成伤害，就会自动触发这个函数
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
    virtual void BeginPlay() override;

};