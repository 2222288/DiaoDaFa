#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "DataAsset/AttackDH.h"
#include "Weapon/WeaponTypes.h"
#include "Base.generated.h"

class AWeaponBase;

UCLASS()
class KERTYER_API ABase : public ACharacter
{
    GENERATED_BODY()

public:
    ABase();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    FName WeaponAttachSocketName = TEXT("hand_r_weapons");

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void SetCurrentWeapon(AWeaponBase* NewWeapon);

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void NotifyWeaponAttackStarted(EAttackDirection AttackDirection, FName AttackType, float AttackStartTime, float BaseDamage, float DamageModifier);

    //UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    //void OpenWeaponContactWindow() { EnableWeaponTrace(); }

    //UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    //void CloseWeaponContactWindow() { DisableWeaponTrace(); }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void EnableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void DisableWeaponTrace();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    float CurrentHealth;

    UFUNCTION(BlueprintCallable, Category = "Attributes")
    void Treat(float HealAmount);

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;



protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Weapon")
    TSubclassOf<AWeaponBase> DefaultWeaponClass;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<AWeaponBase> CurrentWeapon;



};