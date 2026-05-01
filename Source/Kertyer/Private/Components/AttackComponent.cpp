#include "Components/AttackComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAsset/AttackDH.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UAttackComponent::UAttackComponent()
{
	// 需要在 Tick 中释放待定攻击，必须开启 Tick
	PrimaryComponentTick.bCanEverTick = true;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	AttackState = EAttackState::Idle;
	AttackSamplingStartTime = -1.f;
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

	const float CurrentTime = World->GetTimeSeconds();

	// 先根据时间线和按键状态刷新状态机
	RefreshAttackState(CurrentTime);

	// 只要离开锁定期，就可以释放待定攻击
	// 这里会覆盖：
	// 1. 连击窗口刚打开
	// 2. 极端情况下攻击已经播完
	if (bHasPendingAttack && !IsLockedState())
	{
		PerformAttack(PendingDirection, PendingTrackScore);
	}

	// 当前攻击完全结束后，允许方向回归 None
	if (!HasActiveAttack(CurrentTime) && !bHasPendingAttack)
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
	AttackSamplingStartTime = CurrentTime;

	ClearSamplingBuffer();
	RefreshAttackState(CurrentTime);
}

void UAttackComponent::EndAttackSampling()
{
	bIsAttackKeyDown = false;
	AttackSamplingStartTime = -1.f;

	LastAcceptedInputDirection = EAttackDirection::None;
	LastAcceptedInputTime = -10000.0f;

	ClearSamplingBuffer();

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	RefreshAttackState(CurrentTime);
}

void UAttackComponent::CacheMouseInput(const FVector2D& Input, float CurrentTime)
{
	// 只有采样态才接收输入
	if (!IsSamplingState())
	{
		return;
	}

	AccumulatedMousePosition += Input;

	FTrackSample TimedSample;
	TimedSample.Position = AccumulatedMousePosition;
	TimedSample.TimeSeconds = CurrentTime;

	// 距离上一个点太小则丢弃
	if (!RawPoints.IsEmpty() &&
		(AccumulatedMousePosition - RawPoints.Last().Position).SizeSquared() < FMath::Square(MinSampleDistance))
	{
		return;
	}

	RawPoints.Add(TimedSample);

	FTrackResult Result;
	Result.bValid = false;
	Result.Direction = EAttackDirection::None;
	Result.TrackScore = 0.0f;

	// 每 2 个点尝试识别一次
	if (RawPoints.Num() % 2 == 0)
	{
		Result = FTrackPreprocessUtils::Transfer(RawPoints, Config);
	}

	if (!Result.bCanTriggerAttack)
	{
		return;
	}

	if (Result.Direction == EAttackDirection::None)
	{
		return;
	}

	if (!CanAcceptAttackInput(Result.Direction, CurrentTime))
	{
		// 同方向连续输入或切换过快，直接清掉当前轨迹
		// 不清的话，后续点继续叠加，仍可能马上再次识别成功
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

	// 只在“锁定期”拦截同向重复抖动
	// 不再永久封死同方向连续出招
	if (IsLockedState() && Direction == CurrentDirection)
	{
		ClearSamplingBuffer();
		return;
	}

	// 锁定期内的异向输入进入待定队列
	if (IsLockedState())
	{
		bHasPendingAttack = true;
		PendingDirection = Direction;
		PendingTrackScore = TrackScore;

		// 当前这段输入已经消费，后续轨迹必须重新积
		ClearSamplingBuffer();

		UE_LOG(LogTemp, Warning, TEXT("记录待定方向: %d, 待定评分: %f"),
			static_cast<int32>(Direction),
			TrackScore);

		return;
	}

	const FAttack* AttackRow = FindAttackRowByDirection(Direction);
	if (!AttackRow || !AttackRow->AttackMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("攻击失败：没有找到方向对应的 AttackRow, Direction=%d"),
			static_cast<int32>(Direction));

		return;
	}

	if (!CacheAnimInstance() || !Anim)
	{
		return;
	}

	float PlayedLength = Anim->Montage_Play(AttackRow->AttackMontage, 1.0f);

	if (PlayedLength <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("攻击失败：Montage 播放失败"));
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



	// 动画确认成功后再更新伤害倍率和时间线
	CurrentDamageModifier = NextAttackDamageModifier;
	NextAttackDamageModifier = TrackScore;

	CurrentWindowTime = AttackRow->WindowTime;
	CurrentAttackStartTime = CurrentTime;
	CurrentAttackEndTime = CurrentAttackStartTime + PlayedLength;
	CurrentDirection = Direction;
	AttackTriggerCounter++;

	UE_LOG(LogTemp, Warning, TEXT("本次攻击方向: %d, 轨迹得分: %f, 下一击伤害倍率: %f"),
		static_cast<int32>(Direction),
		TrackScore,
		NextAttackDamageModifier);

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

	const float Window = FMath::Max(0.1f, CurrentWindowTime);
	const float WindowStartTime = CurrentAttackStartTime + Window;

	// 修复边界空点：窗口开启吃等号
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

bool UAttackComponent::IsComboWindowState() const
{
	return AttackState == EAttackState::ComboWindowOpen
		|| AttackState == EAttackState::SamplingComboWindow;
}

void UAttackComponent::ClearSamplingBuffer()
{
	RawPoints.Empty();
	AccumulatedMousePosition = FVector2D::ZeroVector;
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
	// 这正是解决“鼠标一直向前移动就一直向前攻击”的关键
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