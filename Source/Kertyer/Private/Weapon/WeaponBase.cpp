#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponContactResolver.h"
#include "Character/Base.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

AWeaponBase::AWeaponBase()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(SceneRoot);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(false);

    WeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
    WeaponCollision->SetupAttachment(WeaponMesh);
    WeaponCollision->SetBoxExtent(FVector(8.0f, 4.0f, 60.0f));
    WeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WeaponCollision->SetGenerateOverlapEvents(true);
    WeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
    WeaponCollision->SetSimulatePhysics(false);
    WeaponCollision->SetEnableGravity(false);

    BladeSocketNames = { TEXT("Blade_Base"), TEXT("Blade_Mid"), TEXT("Blade_Tip") };
}

void AWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    CurrentWeaponState = EWeaponState::Idle;
    bIsTracing = false;
    ResetSocketTracePositions();
}

void AWeaponBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsTracing)
    {
        PerformSocketSweeps(DeltaSeconds);
    }
}

void AWeaponBase::SetCurrentHolder(ABase* NewHolder)
{
    CurrentHolder = NewHolder;
    SetOwner(NewHolder);
}

void AWeaponBase::ReceiveAttackData(const FWeaponAttackData& AttackData)
{
    CurrentAttackData = AttackData;
    CurrentAttackDirection = AttackData.AttackDirection;
    CurrentWeaponState = EWeaponState::Attacking;
    HitActorsThisTrace.Empty();
    ContactedWeaponsThisTrace.Empty();

    if (CurrentAttackData.AttackStartTime < 0.0f)
    {
        if (const UWorld* World = GetWorld())
        {
            CurrentAttackData.AttackStartTime = World->GetTimeSeconds();
        }
    }
}

void AWeaponBase::EnableWeaponTrace()
{
    if (!WeaponMesh)
    {
        return;
    }

    bIsTracing = true;
    CurrentWeaponState = EWeaponState::ContactWindowOpen;
    HitActorsThisTrace.Empty();
    ContactedWeaponsThisTrace.Empty();
    ResetSocketTracePositions();

    UE_LOG(LogTemp, Warning, TEXT("WeaponTrace opened: %s, Direction=%d, Damage=%f"), *GetName(), static_cast<int32>(CurrentAttackDirection), GetCurrentAttackDamage());
}

void AWeaponBase::DisableWeaponTrace()
{
    bIsTracing = false;
    PreviousSocketLocations.Empty();

    if (CurrentWeaponState != EWeaponState::Disabled)
    {
        CurrentWeaponState = EWeaponState::Recovering;
    }

    UE_LOG(LogTemp, Warning, TEXT("WeaponTrace closed: %s"), *GetName());
}

void AWeaponBase::ResetSocketTracePositions()
{
    PreviousSocketLocations.Empty();

    if (!WeaponMesh)
    {
        return;
    }

    for (const FName& SocketName : BladeSocketNames)
    {
        if (WeaponMesh->DoesSocketExist(SocketName))
        {
            PreviousSocketLocations.Add(SocketName, WeaponMesh->GetSocketLocation(SocketName));
        }
    }
}

void AWeaponBase::PerformSocketSweeps(float DeltaSeconds)
{
    if (!WeaponMesh || !GetWorld())
    {
        return;
    }

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponSocketSweep), false, this);
    QueryParams.AddIgnoredActor(this);
    if (CurrentHolder)
    {
        QueryParams.AddIgnoredActor(CurrentHolder);
    }

    const FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceSphereRadius);

    for (const FName& SocketName : BladeSocketNames)
    {
        if (!WeaponMesh->DoesSocketExist(SocketName))
        {
            continue;
        }

        const FVector CurrentLocation = WeaponMesh->GetSocketLocation(SocketName);
        const FVector* PreviousLocationPtr = PreviousSocketLocations.Find(SocketName);

        if (!PreviousLocationPtr)
        {
            PreviousSocketLocations.Add(SocketName, CurrentLocation);
            continue;
        }

        const FVector PreviousLocation = *PreviousLocationPtr;
        PreviousSocketLocations[SocketName] = CurrentLocation;

        if (FVector::DistSquared(PreviousLocation, CurrentLocation) <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        TArray<FHitResult> Hits;
        const bool bHit = GetWorld()->SweepMultiByObjectType(
            Hits,
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

        for (const FHitResult& Hit : Hits)
        {
            ProcessSweepHit(Hit);
        }
    }
}

void AWeaponBase::ProcessSweepHit(const FHitResult& Hit)
{
    AActor* HitActor = Hit.GetActor();
    if (!HitActor || HitActor == this || HitActor == CurrentHolder)
    {
        return;
    }

    if (AWeaponBase* OtherWeapon = Cast<AWeaponBase>(HitActor))
    {
        HandleWeaponHit(OtherWeapon, Hit);
        return;
    }

    if (const UActorComponent* HitComponent = Hit.GetComponent())
    {
        if (AWeaponBase* OtherWeapon = Cast<AWeaponBase>(HitComponent->GetOwner()))
        {
            HandleWeaponHit(OtherWeapon, Hit);
            return;
        }
    }

    if (ABase* HitBody = Cast<ABase>(HitActor))
    {
        HandleBodyHit(HitBody, Hit);
    }
}

void AWeaponBase::HandleBodyHit(ABase* HitBody, const FHitResult& Hit)
{
    if (!HitBody || HitBody == CurrentHolder || HitActorsThisTrace.Contains(HitBody))
    {
        return;
    }

    HitActorsThisTrace.Add(HitBody);

    const float Damage = GetCurrentAttackDamage();
    AController* InstigatorController = CurrentHolder ? CurrentHolder->GetController() : nullptr;

    UGameplayStatics::ApplyDamage(
        HitBody,
        Damage,
        InstigatorController,
        this,
        UDamageType::StaticClass()
    );

    BP_OnBodyHit(HitBody, Hit, Damage);
    UE_LOG(LogTemp, Warning, TEXT("Weapon body hit: Weapon=%s, Target=%s, Damage=%f"), *GetName(), *HitBody->GetName(), Damage);
}

void AWeaponBase::HandleWeaponHit(AWeaponBase* OtherWeapon, const FHitResult& Hit)
{
    if (!OtherWeapon || OtherWeapon == this || OtherWeapon->CurrentHolder == CurrentHolder || ContactedWeaponsThisTrace.Contains(OtherWeapon))
    {
        return;
    }

    ContactedWeaponsThisTrace.Add(OtherWeapon);

    const EWeaponContactResult Result = UWeaponContactResolver::ResolveWeaponContact(this, OtherWeapon);
    ApplyContactResultToWeapons(OtherWeapon, Result);

    BP_OnWeaponContact(OtherWeapon, Result, Hit);
    OtherWeapon->BP_OnWeaponContact(this, Result, Hit);

    if (Result == EWeaponContactResult::Hit && OtherWeapon->CurrentHolder)
    {
        HandleBodyHit(OtherWeapon->CurrentHolder, Hit);
    }

    UE_LOG(LogTemp, Warning, TEXT("Weapon contact: %s vs %s -> %d"), *GetName(), *OtherWeapon->GetName(), static_cast<int32>(Result));
}

void AWeaponBase::ApplyContactResultToWeapons(AWeaponBase* OtherWeapon, EWeaponContactResult Result)
{
    if (!OtherWeapon)
    {
        return;
    }

    switch (Result)
    {
    case EWeaponContactResult::Clash:
        CurrentWeaponState = EWeaponState::Recovering;
        OtherWeapon->CurrentWeaponState = EWeaponState::Recovering;
        bIsTracing = false;
        OtherWeapon->bIsTracing = false;
        break;

    case EWeaponContactResult::Deflect:
    {
        AWeaponBase* SlowerWeapon = CurrentAttackData.AttackStartTime >= OtherWeapon->CurrentAttackData.AttackStartTime ? this : OtherWeapon;
        SlowerWeapon->CurrentWeaponState = EWeaponState::Recovering;
        SlowerWeapon->bIsTracing = false;
        break;
    }

    case EWeaponContactResult::Interrupt:
    {
        AWeaponBase* SlowerWeapon = CurrentAttackData.AttackStartTime >= OtherWeapon->CurrentAttackData.AttackStartTime ? this : OtherWeapon;
        SlowerWeapon->CurrentWeaponState = EWeaponState::Disabled;
        SlowerWeapon->bIsTracing = false;
        break;
    }

    case EWeaponContactResult::Hit:
    case EWeaponContactResult::Ignore:
    default:
        break;
    }
}