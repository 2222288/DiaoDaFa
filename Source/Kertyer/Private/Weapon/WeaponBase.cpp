#include "Weapon/WeaponBase.h"
#include "Weapon/WeaponContactResolver.h"
#include "Character/Base.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const TCHAR* WeaponDirectionToChinese(EAttackDirection Direction)
    {
        switch (Direction)
        {
        case EAttackDirection::None: return TEXT("无");
        case EAttackDirection::Up: return TEXT("上");
        case EAttackDirection::UpRight: return TEXT("右上");
        case EAttackDirection::Right: return TEXT("右");
        case EAttackDirection::DownRight: return TEXT("右下");
        case EAttackDirection::Down: return TEXT("下");
        case EAttackDirection::DownLeft: return TEXT("左下");
        case EAttackDirection::Left: return TEXT("左");
        case EAttackDirection::UpLeft: return TEXT("左上");
        default: return TEXT("未知方向");
        }
    }

    const TCHAR* WeaponStateToChinese(EWeaponState State)
    {
        switch (State)
        {
        case EWeaponState::Idle: return TEXT("空闲");
        case EWeaponState::Attacking: return TEXT("攻击中");
        case EWeaponState::ContactWindowOpen: return TEXT("接触窗口打开");
        case EWeaponState::Recovering: return TEXT("恢复中");
        case EWeaponState::Disabled: return TEXT("失效");
        default: return TEXT("未知状态");
        }
    }

    const TCHAR* WeaponContactResultToChinese(EWeaponContactResult Result)
    {
        switch (Result)
        {
        case EWeaponContactResult::Clash: return TEXT("同时对撞");
        case EWeaponContactResult::Deflect: return TEXT("偏斜");
        case EWeaponContactResult::Interrupt: return TEXT("打断");
        case EWeaponContactResult::Hit: return TEXT("命中");
        case EWeaponContactResult::Ignore: return TEXT("忽略");
        default: return TEXT("未知结果");
        }
    }

    FString SafeObjectName(const UObject* Object)
    {
        return IsValid(Object) ? Object->GetName() : TEXT("无");
    }

    FString SafeAttackName(FName Name)
    {
        return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
    }
}

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

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][武器接收攻击数据] 持有者=%s 武器=%s 方向=%s 攻击ID=%s 开始时间=%.3f 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 武器状态=%s"),
        *SafeObjectName(CurrentHolder),
        *GetName(),
        WeaponDirectionToChinese(CurrentAttackDirection),
        *SafeAttackName(CurrentAttackData.AttackType),
        CurrentAttackData.AttackStartTime,
        CurrentAttackData.BaseDamage,
        CurrentAttackData.DamageModifier,
        GetCurrentAttackDamage(),
        WeaponStateToChinese(CurrentWeaponState));
}

void AWeaponBase::EnableWeaponTrace()
{
    if (!WeaponMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[攻击交互][判定窗口打开失败] 原因=WeaponMesh为空 武器=%s 持有者=%s"),
            *GetName(),
            *SafeObjectName(CurrentHolder));
        return;
    }

    bIsTracing = true;
    CurrentWeaponState = EWeaponState::ContactWindowOpen;

    HitActorsThisTrace.Empty();
    ContactedWeaponsThisTrace.Empty();

    ResetSocketTracePositions();

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][判定窗口打开] 持有者=%s 武器=%s 方向=%s 攻击ID=%s 最终伤害=%.2f Socket数量=%d Trace半径=%.2f 武器状态=%s"),
        *SafeObjectName(CurrentHolder),
        *GetName(),
        WeaponDirectionToChinese(CurrentAttackDirection),
        *SafeAttackName(CurrentAttackData.AttackType),
        GetCurrentAttackDamage(),
        BladeSocketNames.Num(),
        TraceSphereRadius,
        WeaponStateToChinese(CurrentWeaponState));
}

void AWeaponBase::DisableWeaponTrace()
{
    const int32 BodyHitCount = HitActorsThisTrace.Num();
    const int32 WeaponContactCount = ContactedWeaponsThisTrace.Num();

    bIsTracing = false;
    PreviousSocketLocations.Empty();

    if (CurrentWeaponState != EWeaponState::Disabled)
    {
        CurrentWeaponState = EWeaponState::Recovering;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][判定窗口关闭] 持有者=%s 武器=%s 方向=%s 攻击ID=%s 本窗口命中身体数=%d 本窗口接触武器数=%d 武器状态=%s"),
        *SafeObjectName(CurrentHolder),
        *GetName(),
        WeaponDirectionToChinese(CurrentAttackDirection),
        *SafeAttackName(CurrentAttackData.AttackType),
        BodyHitCount,
        WeaponContactCount,
        WeaponStateToChinese(CurrentWeaponState));
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
    if (!bIsTracing)
    {
        return;
    }

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

    const UPrimitiveComponent* HitComponent = Hit.GetComponent();

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][命中敌方身体] 攻击者=%s 攻击武器=%s 受击者=%s 方向=%s 攻击ID=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 命中组件=%s 命中点=%s 命中法线=%s 本窗口已命中身体数=%d"),
        *SafeObjectName(CurrentHolder),
        *GetName(),
        *SafeObjectName(HitBody),
        WeaponDirectionToChinese(CurrentAttackDirection),
        *SafeAttackName(CurrentAttackData.AttackType),
        CurrentAttackData.BaseDamage,
        CurrentAttackData.DamageModifier,
        Damage,
        *SafeObjectName(HitComponent),
        *Hit.Location.ToCompactString(),
        *Hit.ImpactNormal.ToCompactString(),
        HitActorsThisTrace.Num());
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

    UE_LOG(LogTemp, Warning,
        TEXT("[攻击交互][武器碰撞] A角色=%s A武器=%s A方向=%s A状态=%s A攻击ID=%s A开始=%.3f A伤害=%.2f | B角色=%s B武器=%s B方向=%s B状态=%s B攻击ID=%s B开始=%.3f B伤害=%.2f | 结果=%s | 命中点=%s | 本窗口武器接触数=%d"),
        *SafeObjectName(CurrentHolder),
        *GetName(),
        WeaponDirectionToChinese(CurrentAttackDirection),
        WeaponStateToChinese(CurrentWeaponState),
        *SafeAttackName(CurrentAttackData.AttackType),
        CurrentAttackData.AttackStartTime,
        GetCurrentAttackDamage(),
        *SafeObjectName(OtherWeapon->CurrentHolder),
        *OtherWeapon->GetName(),
        WeaponDirectionToChinese(OtherWeapon->CurrentAttackDirection),
        WeaponStateToChinese(OtherWeapon->CurrentWeaponState),
        *SafeAttackName(OtherWeapon->CurrentAttackData.AttackType),
        OtherWeapon->CurrentAttackData.AttackStartTime,
        OtherWeapon->GetCurrentAttackDamage(),
        WeaponContactResultToChinese(Result),
        *Hit.Location.ToCompactString(),
        ContactedWeaponsThisTrace.Num());
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