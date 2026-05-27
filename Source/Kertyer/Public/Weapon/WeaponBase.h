#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponBase.generated.h"

class ABase;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class KERTYER_API AWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    AWeaponBase();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void SetCurrentHolder(ABase* NewHolder);

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    ABase* GetCurrentHolder() const { return CurrentHolder; }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void ReceiveAttackData(const FWeaponAttackData& AttackData);

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void EnableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void DisableWeaponTrace();

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void OpenWeaponContactWindow() { EnableWeaponTrace(); }

    UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
    void CloseWeaponContactWindow() { DisableWeaponTrace(); }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    bool IsWeaponTracing() const { return bIsTracing; }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    EAttackDirection GetCurrentAttackDirection() const { return CurrentAttackDirection; }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    EWeaponState GetWeaponState() const { return CurrentWeaponState; }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    float GetWeaponWeight() const { return WeaponWeight; }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    float GetContactStrength() const { return WeaponContactStrength; }

    UFUNCTION(BlueprintPure, Category = "Combat|Weapon")
    float GetCurrentAttackDamage() const { return CurrentAttackData.GetFinalDamage(); }

    const FWeaponAttackData& GetCurrentAttackData() const { return CurrentAttackData; }

protected:
    virtual void BeginPlay() override;

    void ResetSocketTracePositions();
    void PerformSocketSweeps(float DeltaSeconds);
    void ProcessSweepHit(const FHitResult& Hit);
    void HandleBodyHit(ABase* HitBody, const FHitResult& Hit);
    void HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit);
    void ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result);

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback")
    void BP_OnBodyHit(ABase* HitBody, const FHitResult& Hit, float Damage);

    UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Weapon|Feedback")
    void BP_OnWeaponContact(AWeaponBase* OtherWeapon, EWeaponContactResult Result, const FHitResult& Hit);

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<UBoxComponent> WeaponCollision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    EWeaponType WeaponType = EWeaponType::Sword;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float WeaponWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
    float WeaponContactStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace")
    float TraceSphereRadius = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon|Trace")
    TArray<FName> BladeSocketNames;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    TObjectPtr<ABase> CurrentHolder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    bool bIsTracing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    EAttackDirection CurrentAttackDirection = EAttackDirection::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    EWeaponState CurrentWeaponState = EWeaponState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon")
    FWeaponAttackData CurrentAttackData;

private:
    TMap<FName, FVector> PreviousSocketLocations;
    TSet<TWeakObjectPtr<AActor>> HitActorsThisTrace;
    TSet<TWeakObjectPtr<AWeaponBase>> ContactedWeaponsThisTrace;
};