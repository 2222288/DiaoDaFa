#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/WeaponTypes.h"
#include "WeaponInteractionComponent.generated.h"

class ABase;
class AWeaponBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UWeaponInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponInteractionComponent();

	virtual void BeginPlay() override;

	void ResetForNewAttack();
	void ResetForNewTraceWindow();

	void HandleTraceHits(const TArray<FHitResult>& Hits);
	void HandleTraceHit(const FHitResult& Hit);

	bool HasConsumedInteraction() const;
	void MarkInteractionConsumed(AActor* ConsumedActor);
	void ForceResolveInteraction(AActor* ConsumedActor);

	AActor* GetConsumedActor() const { return ConsumedActorThisTrace.Get(); }
	int32 GetBodyHitCount() const { return HitActorsThisTrace.Num(); }
	int32 GetWeaponContactCount() const { return ContactedWeaponsThisTrace.Num(); }

private:
	AWeaponBase* GetOwnerWeapon() const;

	void HandleBodyHit(ABase* HitBody, const FHitResult& Hit);
	void HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit);

	void ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result);

	bool ShouldIgnoreBodyHitByCounterWindow(
		ABase* HitBody,
		const FHitResult& Hit,
		float& OutElapsed,
		float& OutWindow,
		FString& OutReason
	) const;

	void ApplyBodyDamageAndInterrupt(
		ABase* TargetBody,
		AWeaponBase* DamageSourceWeapon,
		const FHitResult& Hit,
		float Damage,
		const FString& Reason,
		bool bSuppressNonLethalHitReaction = false,
		bool bInterruptTarget = true
	);

	AWeaponBase* FindWeaponFromHit(const FHitResult& Hit) const;
	ABase* FindBodyFromHit(const FHitResult& Hit) const;

private:
	TSet<TObjectPtr<ABase>> HitActorsThisTrace;

	TSet<TObjectPtr<AWeaponBase>> ContactedWeaponsThisTrace;

	bool bHasResolvedInteractionThisTrace = false;

	TWeakObjectPtr<AActor> ConsumedActorThisTrace;

	TSet<TWeakObjectPtr<ABase>> IgnoredBodyActorsThisTraceForLog;
};