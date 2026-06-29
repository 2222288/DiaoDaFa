#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponTraceComponent.generated.h"

class AWeaponBase;

/**
 * 每个 Tick 只广播一次已经稳定排序的命中批次。
 * 交互层按该顺序解析，并在一次交互被消费后停止处理后续命中。
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FWeaponTraceHitsSignature, const TArray<FHitResult>&);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class KERTYER_API UWeaponTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponTraceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	void EnableTrace();
	void DisableTrace();
	void ForceStopTrace();

	bool IsTracing() const { return bIsTracing; }

	FWeaponTraceHitsSignature OnWeaponTraceHits;

private:
	AWeaponBase* GetOwnerWeapon() const;

	void ResetSocketTracePositions();
	void PerformSocketSweeps(float DeltaSeconds);

	AActor* ResolveSupportedHitActor(const FHitResult& Hit) const;
	bool IsWeaponHit(const FHitResult& Hit) const;

private:
	TMap<FName, FVector> PreviousSocketLocations;

	bool bIsTracing = false;
};
