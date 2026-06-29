#include "Components/WeaponTraceComponent.h"

#include "Character/Base.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Weapon/WeaponBase.h"

namespace
{
	struct FStableWeaponTraceCandidate
	{
		FHitResult Hit;
		AActor* CanonicalActor = nullptr;
		bool bWeaponHit = false;
		int32 SocketIndex = INDEX_NONE;
		uint32 ActorStableId = MAX_uint32;
		uint32 ComponentStableId = MAX_uint32;
	};

	bool IsCandidateBefore(
		const FStableWeaponTraceCandidate& Left,
		const FStableWeaponTraceCandidate& Right
	)
	{
		// 1. 武器碰撞优先于身体命中。
		if (Left.bWeaponHit != Right.bWeaponHit)
		{
			return Left.bWeaponHit;
		}

		// 2. 同一帧中更早发生的 Sweep 命中优先。
		if (!FMath::IsNearlyEqual(Left.Hit.Time, Right.Hit.Time))
		{
			return Left.Hit.Time < Right.Hit.Time;
		}

		// 3. 从对应 Socket Sweep 起点算起，距离更近的命中优先。
		if (!FMath::IsNearlyEqual(Left.Hit.Distance, Right.Hit.Distance))
		{
			return Left.Hit.Distance < Right.Hit.Distance;
		}

		// 4. 用运行期稳定 ID 消除容器和引擎返回顺序造成的抖动。
		if (Left.ActorStableId != Right.ActorStableId)
		{
			return Left.ActorStableId < Right.ActorStableId;
		}

		if (Left.ComponentStableId != Right.ComponentStableId)
		{
			return Left.ComponentStableId < Right.ComponentStableId;
		}

		// 最后才使用 Socket 配置顺序，确保比较器具有严格全序。
		return Left.SocketIndex < Right.SocketIndex;
	}
}

UWeaponTraceComponent::UWeaponTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWeaponTraceComponent::BeginPlay()
{
	Super::BeginPlay();

	bIsTracing = false;
	SetComponentTickEnabled(false);
	ResetSocketTracePositions();
}

void UWeaponTraceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsTracing)
	{
		SetComponentTickEnabled(false);
		return;
	}

	AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	if (!OwnerWeapon || OwnerWeapon->HasResolvedInteractionThisTrace())
	{
		// 窗口逻辑可以继续保持开启，但本轮交互完成后不再产生无意义 Sweep。
		SetComponentTickEnabled(false);
		return;
	}

	PerformSocketSweeps(DeltaTime);
}

void UWeaponTraceComponent::EnableTrace()
{
	ResetSocketTracePositions();
	bIsTracing = true;
	SetComponentTickEnabled(true);
}

void UWeaponTraceComponent::DisableTrace()
{
	bIsTracing = false;
	SetComponentTickEnabled(false);
	PreviousSocketLocations.Empty();
}

void UWeaponTraceComponent::ForceStopTrace()
{
	bIsTracing = false;
	SetComponentTickEnabled(false);
	PreviousSocketLocations.Empty();
}

AWeaponBase* UWeaponTraceComponent::GetOwnerWeapon() const
{
	return Cast<AWeaponBase>(GetOwner());
}

void UWeaponTraceComponent::ResetSocketTracePositions()
{
	PreviousSocketLocations.Empty();

	const AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	if (!OwnerWeapon || !OwnerWeapon->WeaponMesh)
	{
		return;
	}

	for (const FName& SocketName : OwnerWeapon->BladeSocketNames)
	{
		if (OwnerWeapon->WeaponMesh->DoesSocketExist(SocketName))
		{
			PreviousSocketLocations.Add(
				SocketName,
				OwnerWeapon->WeaponMesh->GetSocketLocation(SocketName)
			);
		}
	}
}

void UWeaponTraceComponent::PerformSocketSweeps(float DeltaSeconds)
{
	(void)DeltaSeconds;

	AWeaponBase* OwnerWeapon = GetOwnerWeapon();
	if (!OwnerWeapon || !OwnerWeapon->WeaponMesh || !GetWorld()
		|| OwnerWeapon->HasResolvedInteractionThisTrace())
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(OwnerWeapon->GetWeaponObjectChannel());

	for (const TEnumAsByte<ECollisionChannel> ObjectChannel
		: OwnerWeapon->GetWeaponTraceObjectChannels())
	{
		ObjectQueryParams.AddObjectTypesToQuery(ObjectChannel.GetValue());
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(WeaponSocketSweep),
		false,
		OwnerWeapon
	);
	QueryParams.AddIgnoredActor(OwnerWeapon);

	if (ABase* CurrentHolder = OwnerWeapon->GetCurrentHolder())
	{
		QueryParams.AddIgnoredActor(CurrentHolder);
	}

	const FCollisionShape SphereShape =
		FCollisionShape::MakeSphere(OwnerWeapon->TraceSphereRadius);

	TArray<FStableWeaponTraceCandidate> Candidates;
	Candidates.Reserve(OwnerWeapon->BladeSocketNames.Num() * 4);

	for (int32 SocketIndex = 0;
		SocketIndex < OwnerWeapon->BladeSocketNames.Num();
		++SocketIndex)
	{
		const FName SocketName = OwnerWeapon->BladeSocketNames[SocketIndex];

		if (!OwnerWeapon->WeaponMesh->DoesSocketExist(SocketName))
		{
			continue;
		}

		const FVector CurrentLocation =
			OwnerWeapon->WeaponMesh->GetSocketLocation(SocketName);
		const FVector* PreviousLocationPtr =
			PreviousSocketLocations.Find(SocketName);

		if (!PreviousLocationPtr)
		{
			PreviousSocketLocations.Add(SocketName, CurrentLocation);
			continue;
		}

		const FVector PreviousLocation = *PreviousLocationPtr;
		PreviousSocketLocations[SocketName] = CurrentLocation;

		if (FVector::DistSquared(PreviousLocation, CurrentLocation)
			<= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		TArray<FHitResult> SocketHits;
		const bool bHit = GetWorld()->SweepMultiByObjectType(
			SocketHits,
			PreviousLocation,
			CurrentLocation,
			FQuat::Identity,
			ObjectQueryParams,
			SphereShape,
			QueryParams
		);

		if (!bHit)
		{
			continue;
		}

		for (const FHitResult& Hit : SocketHits)
		{
			AActor* CanonicalActor = ResolveSupportedHitActor(Hit);
			if (!CanonicalActor || CanonicalActor == OwnerWeapon
				|| CanonicalActor == OwnerWeapon->GetCurrentHolder())
			{
				continue;
			}

			FStableWeaponTraceCandidate& Candidate =
				Candidates.AddDefaulted_GetRef();
			Candidate.Hit = Hit;
			Candidate.CanonicalActor = CanonicalActor;
			Candidate.bWeaponHit = IsWeaponHit(Hit);
			Candidate.SocketIndex = SocketIndex;
			Candidate.ActorStableId = CanonicalActor->GetUniqueID();

			if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
			{
				Candidate.ComponentStableId = HitComponent->GetUniqueID();
			}
		}
	}

	if (Candidates.IsEmpty())
	{
		return;
	}

	Candidates.Sort(IsCandidateBefore);

	// 同一 Actor 被多个 Socket 或多个组件命中时，只保留排序后的第一个结果。
	TSet<const AActor*> AddedActors;
	TArray<FHitResult> StableHits;
	StableHits.Reserve(Candidates.Num());

	for (const FStableWeaponTraceCandidate& Candidate : Candidates)
	{
		if (!Candidate.CanonicalActor || AddedActors.Contains(Candidate.CanonicalActor))
		{
			continue;
		}

		AddedActors.Add(Candidate.CanonicalActor);
		StableHits.Add(Candidate.Hit);
	}

	if (!StableHits.IsEmpty())
	{
		// 每帧只进行一次跨层调用，交互层统一解析。
		OnWeaponTraceHits.Broadcast(StableHits);
	}
}

AActor* UWeaponTraceComponent::ResolveSupportedHitActor(const FHitResult& Hit) const
{
	if (AWeaponBase* HitWeapon = Cast<AWeaponBase>(Hit.GetActor()))
	{
		return HitWeapon;
	}

	if (ABase* HitBody = Cast<ABase>(Hit.GetActor()))
	{
		return HitBody;
	}

	if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		if (AWeaponBase* HitWeapon = Cast<AWeaponBase>(HitComponent->GetOwner()))
		{
			return HitWeapon;
		}

		if (ABase* HitBody = Cast<ABase>(HitComponent->GetOwner()))
		{
			return HitBody;
		}
	}

	// 当前交互解析器还没有可破坏物接口，先过滤掉无法结算的对象。
	return nullptr;
}

bool UWeaponTraceComponent::IsWeaponHit(const FHitResult& Hit) const
{
	if (Cast<AWeaponBase>(Hit.GetActor()))
	{
		return true;
	}

	if (const UPrimitiveComponent* HitComponent = Hit.GetComponent())
	{
		return Cast<AWeaponBase>(HitComponent->GetOwner()) != nullptr;
	}

	return false;
}
