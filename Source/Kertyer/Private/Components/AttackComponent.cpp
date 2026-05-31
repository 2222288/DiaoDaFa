#include "Components/AttackComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAsset/AttackDH.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Character/Base.h"
#include "GameFramework/Character.h"

namespace
{
	const TCHAR* AttackDirectionToChinese(EAttackDirection Direction)
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

	const TCHAR* AttackStateToChinese(EAttackState State)
	{
		switch (State)
		{
		case EAttackState::Idle: return TEXT("空闲");
		case EAttackState::Sampling: return TEXT("采样中");
		case EAttackState::AttackingLocked: return TEXT("攻击锁定");
		case EAttackState::ComboWindowOpen: return TEXT("连击窗口打开");
		case EAttackState::SamplingLocked: return TEXT("采样但攻击锁定");
		case EAttackState::SamplingComboWindow: return TEXT("采样且连击窗口打开");
		default: return TEXT("未知状态");
		}
	}

	FString SafeActorName(const AActor* Actor)
	{
		return IsValid(Actor) ? Actor->GetName() : TEXT("无");
	}

	FString SafeNameText(FName Name)
	{
		return Name.IsNone() ? FString(TEXT("无")) : Name.ToString();
	}
}


UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	AttackState = EAttackState::Idle;
	CurrentAttackStartTime = -1.f;
	CurrentAttackEndTime = -1.f;
	CurrentWindowTime = 0.0f;
	CurrentDirection = EAttackDirection::None;

	ClearPendingAttack();
	ClearSamplingBuffer();
}

void UAttackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	//该帧时间
	const float CurrentTime = World->GetTimeSeconds();

	//实时监测状态机
	RefreshAttackState(CurrentTime);

	// 待定攻击处理
	if (bHasPendingAttack && !IsLockedState())
	{
		PerformAttack(PendingDirection, PendingTrackScore);
	}

	//当前是否正在攻击
	const bool bAttackActive = HasActiveAttack(CurrentTime);


	if (!bAttackActive && bWeaponTraceWindowOpen)
	{
		DisableWeaponTrace();
	}

	if (!bAttackActive && !bHasPendingAttack)
	{
		CurrentDirection = EAttackDirection::None;
	}
}

void UAttackComponent::BeginAttackSampling(float CurrentTime)
{
	RefreshAttackState(CurrentTime);

	// 已经按住了就不重复初始化
	if (bIsAttackKeyDown)
	{
		return;
	}

	// 锁定期内不允许开始新的采样
	if (IsLockedState())
	{
		return;
	}

	if (!CacheAnimInstance())
	{
		return;
	}

	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;

	bIsAttackKeyDown = true;
	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

void UAttackComponent::EndAttackSampling()
{
	bIsAttackKeyDown = false;

	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;

	ClearSamplingBuffer();

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	RefreshAttackState(CurrentTime);
}

void UAttackComponent::CacheMouseInput(const FVector2D& Input, float CurrentTime)
{
	// 只有采样态才接收输入。组件只负责把输入交给 AttackValid，
	// 不再直接维护 RawPoints、累计坐标或采样窗口。
	if (!IsSamplingState())
	{
		return;
	}

	FAttackValidResult Result;
	if (!AttackValid.PushInput(Input, CurrentTime, MinSampleDistance, Result))
	{
		return;
	}

	if (!Result.bCanTriggerAttack || Result.Direction == EAttackDirection::None)
	{
		return;
	}

	if (!CanAcceptAttackInput(Result.Direction, CurrentTime))
	{
		// 同方向连续输入或切换过快，直接清掉当前轨迹。
		// 不清的话，后续点继续叠加，仍可能马上再次识别成功。
		ClearSamplingBuffer();
		return;
	}

	MarkAttackInputAccepted(Result.Direction, CurrentTime);
	PerformAttack(Result.Direction, Result.TrackScore);
}

void UAttackComponent::PerformAttack(EAttackDirection Direction, float TrackScore)
{
	if (Direction == EAttackDirection::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	RefreshAttackState(CurrentTime);

	if (IsLockedState() && Direction == CurrentDirection)
	{
		ClearSamplingBuffer();

		return;
	}

	if (IsLockedState())
	{
		bHasPendingAttack = true;
		PendingDirection = Direction;
		PendingTrackScore = TrackScore;

		ClearSamplingBuffer();

		return;
	}

	const FAttack* AttackRow = FindAttackRowByDirection(Direction);
	if (!AttackRow || !AttackRow->AttackMontage)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[攻击交互][出招失败] 原因=未找到AttackRow或Montage为空 角色=%s 方向=%d"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			static_cast<int32>(Direction));

		return;
	}

	if (!CacheAnimInstance() || !Anim)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[攻击交互][出招失败] 原因=AnimInstance为空 角色=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"));

		return;
	}

	float PlayedLength = Anim->Montage_Play(AttackRow->AttackMontage, 1.0f);

	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[攻击交互][出招失败] 原因=Montage播放失败 角色=%s Montage=%s"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
			*GetNameSafe(AttackRow->AttackMontage));

		return;
	}

	if (AttackRow->MontageSection != NAME_None)
	{
		Anim->Montage_JumpToSection(AttackRow->MontageSection, AttackRow->AttackMontage);

		const int32 SectionIndex = AttackRow->AttackMontage->GetSectionIndex(AttackRow->MontageSection);
		if (SectionIndex != INDEX_NONE)
		{
			PlayedLength = AttackRow->AttackMontage->GetSectionLength(SectionIndex);
		}
	}

	CurrentBaseDamage = AttackRow->Damage;
	CurrentDamageModifier = NextAttackDamageModifier;
	NextAttackDamageModifier = TrackScore;
	CurrentWindowTime = AttackRow->WindowTime;
	CurrentAttackStartTime = CurrentTime;
	CurrentAttackEndTime = CurrentAttackStartTime + PlayedLength;
	CurrentDirection = Direction;
	CurrentAttackType = AttackRow->AttackID;
	bWeaponTraceWindowOpen = false;
	AttackTriggerCounter++;

	if (ABase* OwnerCharacter = Cast<ABase>(GetOwner()))
	{
		OwnerCharacter->NotifyWeaponAttackStarted(
			Direction,
			CurrentAttackType,
			CurrentAttackStartTime,
			CurrentBaseDamage,
			CurrentDamageModifier,
			CounterAttackValidWindow
		);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][出招成功] 角色=%s 方向=%d 攻击ID=%s Montage=%s Section=%s 基础伤害=%.2f 倍率=%.3f 最终伤害=%.2f 攻击开始=%.3f 攻击时长=%.3f 响应窗口=%.3f 触发计数=%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
		static_cast<int32>(Direction),
		*CurrentAttackType.ToString(),
		*GetNameSafe(AttackRow->AttackMontage),
		*AttackRow->MontageSection.ToString(),
		CurrentBaseDamage,
		CurrentDamageModifier,
		GetCurrentAttackDamage(),
		CurrentAttackStartTime,
		PlayedLength,
		CounterAttackValidWindow,
		AttackTriggerCounter);

	ClearPendingAttack();
	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

const FAttack* UAttackComponent::FindAttackRowByDirection(EAttackDirection InDirection) const
{
	if (!AttackDataTable || InDirection == EAttackDirection::None)
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("FindAttackRowByDirection"));

	const TArray<FName> RowNames = AttackDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		const FAttack* Row = AttackDataTable->FindRow<FAttack>(RowName, ContextString);
		if (!Row)
		{
			continue;
		}

		if (Row->AttackDirection == InDirection)
		{
			return Row;
		}
	}

	return nullptr;
}

void UAttackComponent::StartBlock()
{
	bIsBlocking = true;
}

void UAttackComponent::StopBlock()
{
	bIsBlocking = false;
}

void UAttackComponent::RefreshAttackState(float CurrentTime)
{
	//当前是否存在正在播放的攻击
	const bool bActiveAttack = HasActiveAttack(CurrentTime);

	if (!bActiveAttack)
	{
		CurrentAttackStartTime = -1.f;
		CurrentAttackEndTime = -1.f;
		CurrentWindowTime = 0.0f;

		// 没有活动攻击时，状态只取决于当前是否按住攻击键
		AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;
		return;
	}

	//窗口期时间
	const float Window = FMath::Max(0.1f, CurrentWindowTime);
	//窗口期结束时间
	const float WindowStartTime = CurrentAttackStartTime + Window;

	// 是否处于窗口期
	const bool bWindowOpen = (CurrentTime >= WindowStartTime);

	if (bIsAttackKeyDown)
	{
		AttackState = bWindowOpen
			? EAttackState::SamplingComboWindow
			: EAttackState::SamplingLocked;
	}
	else
	{
		AttackState = bWindowOpen
			? EAttackState::ComboWindowOpen
			: EAttackState::AttackingLocked;
	}
}

bool UAttackComponent::HasActiveAttack(float CurrentTime) const
{
	return CurrentAttackEndTime > 0.0f && CurrentTime < CurrentAttackEndTime;
}

bool UAttackComponent::IsSamplingState() const
{
	return AttackState == EAttackState::Sampling
		|| AttackState == EAttackState::SamplingLocked
		|| AttackState == EAttackState::SamplingComboWindow;
}

bool UAttackComponent::IsLockedState() const
{
	return AttackState == EAttackState::AttackingLocked
		|| AttackState == EAttackState::SamplingLocked;
}

void UAttackComponent::ClearSamplingBuffer()
{
	AttackValid.Reset();
}

void UAttackComponent::ClearPendingAttack()
{
	bHasPendingAttack = false;
	PendingDirection = EAttackDirection::None;
	PendingTrackScore = 0.0f;
}

bool UAttackComponent::CacheAnimInstance()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return false;
	}

	USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh();
	if (!OwnerMesh)
	{
		return false;
	}

	Anim = OwnerMesh->GetAnimInstance();
	return Anim != nullptr;
}

bool UAttackComponent::CanAcceptAttackInput(EAttackDirection Direction, float CurrentTime) const
{
	if (Direction == EAttackDirection::None)
	{
		return false;
	}

	// 没有历史输入，允许
	if (LastAcceptedInputDirection == EAttackDirection::None)
	{
		return true;
	}

	const float Elapsed = CurrentTime - LastAcceptedInputTime;

	// 同方向连续输入直接忽略
	if (Direction == LastAcceptedInputDirection)
	{
		return false;
	}

	// 不同方向也必须满足切换间隔
	if (Elapsed < DirectionSwitchCooldown)
	{
		return false;
	}

	// 额外保护：任意攻击请求之间不能太密
	if (Elapsed < AttackRequestCooldown)
	{
		return false;
	}

	return true;
}

void UAttackComponent::MarkAttackInputAccepted(EAttackDirection Direction, float CurrentTime)
{
	LastAcceptedInputDirection = Direction;
	LastAcceptedInputTime = CurrentTime;
}

bool UAttackComponent::IsAttackActive() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return HasActiveAttack(World->GetTimeSeconds());
}

float UAttackComponent::GetCurrentAttackDamage() const
{
	return CurrentBaseDamage * CurrentDamageModifier;
}

void UAttackComponent::EnableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = true;
	OwnerCharacter->EnableWeaponTrace();
}

void UAttackComponent::DisableWeaponTrace()
{
	ABase* OwnerCharacter = Cast<ABase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	bWeaponTraceWindowOpen = false;
	OwnerCharacter->DisableWeaponTrace();
}

void UAttackComponent::InterruptCurrentAttack()
{
	if (CacheAnimInstance() && Anim)
	{
		Anim->Montage_Stop(0.10f);
	}

	CurrentAttackStartTime = -1.0f;
	CurrentAttackEndTime = -1.0f;
	CurrentWindowTime = 0.0f;
	CurrentDirection = EAttackDirection::None;
	CurrentAttackType = NAME_None;
	CurrentBaseDamage = 0.0f;
	bWeaponTraceWindowOpen = false;

	ClearPendingAttack();
	ClearSamplingBuffer();

	AttackState = bIsAttackKeyDown ? EAttackState::Sampling : EAttackState::Idle;

	UE_LOG(LogTemp, Warning,
		TEXT("[攻击交互][攻击组件被打断] 角色=%s 新状态=%d"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("无"),
		static_cast<int32>(AttackState));
}