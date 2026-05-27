#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/StaticMeshComponent.h"
#include "DataAsset/AttackDH.h"
#include "Base.generated.h"

class AWeaponBase;

UCLASS()
class KERTYER_API ABase : public ACharacter
{
    GENERATED_BODY()

public:
    ABase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Legacy")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon")
    TSubclassOf<AWeaponBase> DefaultWeaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    FName WeaponAttachSocketName = TEXT("hand_r_weapons");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<AWeaponBase> CurrentWeapon;

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void SetCurrentWeapon(AWeaponBase* NewWeapon);

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier);

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void EnableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void DisableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void OpenWeaponContactWindow() { EnableWeaponTrace(); }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void CloseWeaponContactWindow() { DisableWeaponTrace(); }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void EnableWeaponDamage();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void DisableWeaponDamage();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float CurrentHealth;

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void Treat(float HealAmount);

    UFUNCTION()
    void OnWeaponOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    TSet<AActor*> HitActors;

    bool bWeaponDamageEnabled = false;
};