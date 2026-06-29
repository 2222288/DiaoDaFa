// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/LockOn.h"

#include "Engine/OverlapResult.h"         
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "CollisionQueryParams.h"
#include "Engine/GameViewportClient.h"
#include "WorldCollision.h"

void ULockOn::ApplyLockOnCursorMode(bool bEnable)
{
	if (!OwnerPC) return;

	if (bEnable)
	{
		FInputModeGameAndUI Mode;
		//保持 false：让 bShowMouseCursor 真正控制显示/隐藏
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		OwnerPC->SetInputMode(Mode);

		if (UWorld* World = OwnerPC->GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			}
		}

		OwnerPC->bEnableClickEvents = false;
		OwnerPC->bEnableMouseOverEvents = false;

		// 锁定时默认隐藏系统光标（但不破坏后续“开关”）
		OwnerPC->bShowMouseCursor = false;

		//不要长期设 None；至少保证默认是 Default，后面开关才好开出来
		OwnerPC->CurrentMouseCursor = EMouseCursor::Default;
	}
	else
	{
		FInputModeGameOnly Mode;
		OwnerPC->SetInputMode(Mode);

		if (UWorld* World = OwnerPC->GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
				ViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
			}
		}

		OwnerPC->SetIgnoreLookInput(false);
		OwnerPC->bEnableClickEvents = false;
		OwnerPC->bEnableMouseOverEvents = false;

		// 解除锁定时你目前也是隐藏
		OwnerPC->bShowMouseCursor = false;
		OwnerPC->CurrentMouseCursor = EMouseCursor::Default;
	}
}

ULockOn::ULockOn()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockOn::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerPC = Cast<APlayerController>(OwnerCharacter->GetController());
	}

	SetComponentTickEnabled(false);
}
//销毁时解除锁定	
void ULockOn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTarget();
	Super::EndPlay(EndPlayReason);
}

//DeltaTime距离上一秒更新的时间
void ULockOn::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//运行中可能发生重新Possess，兜底更新控制器
	if (!OwnerPC && OwnerCharacter)
	{
		OwnerPC = Cast<APlayerController>(OwnerCharacter->GetController());
	}

	if (SwitchCooldown > 0.0f)
	{
		SwitchCooldown = FMath::Max(0.0f, SwitchCooldown - DeltaTime);
	}

	if (!bLockedOn) return;

	if (!ValidateLockedTarget(DeltaTime))
	{
		ClearTarget();
		return;
	}

	UpdateLockOn(DeltaTime);
}

void ULockOn::ToggleLockOn()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(GetOwner());
	}

	if (!OwnerPC && OwnerCharacter)
	{
		OwnerPC = Cast<APlayerController>(OwnerCharacter->GetController());
	}

	if (!OwnerCharacter || !OwnerPC) return;

	if (bLockedOn)
	{
		ClearTarget();
		return;
	}

	// 只有真正获取到目标时，才进入锁定并切换输入/鼠标模式
	if (AcquireTarget())
	{
		bLockedOn = true;
		OccludedAccum = 0.0f;

		OwnerPC->SetIgnoreLookInput(true);

		// 角色朝向锁定目标（使用控制器Yaw）
		OwnerCharacter->bUseControllerRotationYaw = true;
		if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = false;
		}

		ApplyLockOnCursorMode(true);
		SetComponentTickEnabled(true);
	}
}

void ULockOn::SwitchTargetLeft()
{
	SwitchTarget(-1);
}

void ULockOn::SwitchTargetRight()
{
	SwitchTarget(+1);
}

//切换目标逻辑 Dir是向左还是向右切换
void ULockOn::SwitchTarget(int32 Dir)
{
	if (!bLockedOn || SwitchCooldown > 0.0f) return;
	if (!OwnerCharacter || !OwnerPC) return;
	if (!CanSwitchLockTarget()) return;

	AActor* Current = LockTarget.Get();
	if (!Current) return;
	//Center屏幕中心位置 Center屏幕对角线
	FVector2D Center; float Diag; int32 W, H;
	if (!GetViewportInfo(Center, Diag, W, H)) return;

	//当前锁定目标在屏幕上的位置
	FVector2D CurrScreen;
	//ProjectWorldLocationToScreen:3D转2D 最后一个参数:是否含有黑边 计算锁定目标在屏幕上的位置
	if (!OwnerPC->ProjectWorldLocationToScreen(GetTargetLockPoint(Current), CurrScreen, true))
	{
		CurrScreen = Center;
	}
	//监测世界环境是否正常
	UWorld* World = GetWorld();
	if (!World) return;
	//动态数组 类型:封装单次物理重叠检测的所有信息
	TArray<FOverlapResult> Overlaps;
	//FCollisionObjectQueryParams:找什么类型的物体
	FCollisionObjectQueryParams ObjParams;
	//添加查询类型
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	//FCollisionQueryParams:查询过程的规则
	FCollisionQueryParams QueryParams;
	//不检查自己
	QueryParams.AddIgnoredActor(OwnerCharacter);
	//OverlapMultiByObjectType 是一个用于获取指定范围内、指定类型的所有重叠物体的函数
	World->OverlapMultiByObjectType(
		Overlaps,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		ObjParams,
		FCollisionShape::MakeSphere(MaxDistanceCm),
		QueryParams
	);

	//存放合法的目标
	AActor* Best = nullptr;
	//分数越小越合法
	float BestKey = 1e9f;

	for (const FOverlapResult& R : Overlaps)
	{
		AActor* C = R.GetActor();
		if (!C || C == OwnerCharacter || C == Current) continue;

		if (!IsValidLockCandidate(C)) continue;
		if (!IsTargetAlive(C)) continue;
		//计算距离
		const float Dist = FVector::Dist(OwnerCharacter->GetActorLocation(), GetTargetLockPoint(C));
		if (Dist > MaxDistanceCm) continue;

		if (!IsInKeepAngle(C)) continue;

		// 切换时更严格：必须当下可见
		if (!IsVisibleToTarget(C)) continue;
		//下一个目标的二维坐标
		FVector2D Screen;
		//ProjectWorldLocationToScreen 三维坐标投影转换成玩家屏幕上的二维像素坐标
		if (!OwnerPC->ProjectWorldLocationToScreen(GetTargetLockPoint(C), Screen, true)) continue;

		// 只在屏幕内切换
		if (Screen.X < 0 || Screen.X > W || Screen.Y < 0 || Screen.Y > H) continue;
		//当前目标与下一个目标的模长(屏幕上)
		const float Dx = Screen.X - CurrScreen.X;
		//动态数组可不是按从左到右排序目标的哦
		if (Dir < 0)
		{
			if (Dx > -MinSwitchPixelDelta) continue;
		}
		else
		{
			if (Dx < MinSwitchPixelDelta) continue;
		}
		//屏幕空间的归一化处理 

		// 相对于上一个目标的水平百分比	位移
		const float AbsDxNorm = FMath::Abs(Dx) / FMath::Max(1.0f, (float)W);
		//计算实际距离(百分数)
		const float DistNorm = Dist / FMath::Max(1.0f, MaxDistanceCm);
		//计算屏幕上的二维距离 .Size()计算向量的模长 向量减法:是一条从屏幕中心指向下一个目标的箭头
		const float CenterNorm = (Screen - Center).Size() / FMath::Max(1.0f, Diag);

		//	优先屏幕，其次距离，再轻微偏向屏幕中心
		const float Key = AbsDxNorm * 0.85f + DistNorm * 0.10f + CenterNorm * 0.05f;

		if (Key < BestKey)
		{
			BestKey = Key;
			Best = C;
		}
	}

	if (Best)
	{
		SetTarget(Best);
		SwitchCooldown = SwitchCooldownSeconds;
	}
}

//获取目标 返回值代表这次锁定尝试是否成功
bool ULockOn::AcquireTarget()
{
	if (!OwnerCharacter || !OwnerPC) return false;

	FVector2D Center; float Diag; int32 W, H;
	if (!GetViewportInfo(Center, Diag, W, H)) return false;

	//监测世界环境是否正常
	UWorld* World = GetWorld();
	if (!World) return false;

	//TArray动态数组 存放检测到的物体
	TArray<FOverlapResult> Overlaps;
	//FCollisionObjectQueryParams查询参数结构体
	FCollisionObjectQueryParams ObjParams;
	//添加查询类型
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	//FCollisionQueryParams查询规则
	FCollisionQueryParams QueryParams;
	//不检查自己
	QueryParams.AddIgnoredActor(OwnerCharacter);
	//OverlapMultiByObjectType 是一个用于获取指定范围内、指定类型的所有重叠物体的函数,bHit判断是否有合法的目标
	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		ObjParams,
		FCollisionShape::MakeSphere(MaxDistanceCm),
		QueryParams
	);

	if (!bHit) return false;
	//目标分数并非都是正数
	float BestScore = -1e9f;

	AActor* Best = nullptr;
	//从 Overlaps 数组中，挨个取出每一个元素，暂命名为 R，直到取完为止 类型与前面前后呼应
	for (const FOverlapResult& R : Overlaps)
	{
		//因为该元素目前是一个结构体的元素,所以需要获取该结构体内的Actor类型进行后续操作
		AActor* C = R.GetActor();
		//判断获取是否成功,失败则下个元素
		if (!C || C == OwnerCharacter) continue;

		if (!IsValidLockCandidate(C)) continue;
		if (!IsTargetAlive(C)) continue;
		//Dist:计算两点之间的距离 GetActorLocation:获取角色当前位置 GetTargetLockPoint:获取锁定点位置
		const float Dist = FVector::Dist(OwnerCharacter->GetActorLocation(), GetTargetLockPoint(C));

		//判断锁定距离是否合法
		if (Dist > MaxDistanceCm) continue;

		//判断角度
		if (!IsInKeepAngle(C)) continue;

		// 获取目标阶段必须可见（按您需求）
		if (!IsVisibleToTarget(C)) continue;

		//分数处理
		const float Score = ScoreForAcquire(C, Center, Diag);
		if (Score > BestScore)
		{
			BestScore = Score;
			//设定目标为C
			Best = C;
		}
	}

	if (Best)
	{
		SetTarget(Best);
		return true;
	}

	return false;
}

//设置锁定目标
void ULockOn::SetTarget(AActor* NewTarget)
{
	if (AActor* Old = LockTarget.Get())
	{
		Old->OnDestroyed.RemoveDynamic(this, &ULockOn::OnTargetDestroyed);
	}

	LockTarget = NewTarget;
	CachedTargetSocket = NAME_None;

	if (NewTarget)
	{
		NewTarget->OnDestroyed.AddDynamic(this, &ULockOn::OnTargetDestroyed);

		USkeletalMeshComponent* Mesh = nullptr;
		if (const ACharacter* AsChar = Cast<ACharacter>(NewTarget))
		{
			Mesh = AsChar->GetMesh();
		}
		else
		{
			Mesh = NewTarget->FindComponentByClass<USkeletalMeshComponent>();
		}

		if (Mesh)
		{
			for (const FName& SocketName : ChestSocketCandidates)
			{
				if (Mesh->DoesSocketExist(SocketName))
				{
					CachedTargetSocket = SocketName;
					break;
				}
			}
		}
	}

	OccludedAccum = 0.0f;

	VisibilityCheckTimer = VisibilityCheckInterval;
	bLastKnownVisibility = true;
}

//解除对象的锁定
void ULockOn::OnTargetDestroyed(AActor* DestroyedActor)
{
	ClearTarget();
}

//被解除锁定逻辑
void ULockOn::ClearTarget()
{
	if (AActor* T = LockTarget.Get())
	{
		//解除对当前目标的监听
		T->OnDestroyed.RemoveDynamic(this, &ULockOn::OnTargetDestroyed);
	}

	LockTarget = nullptr;

	bLockedOn = false;
	OccludedAccum = 0.0f;
	SwitchCooldown = 0.0f;


	if (OwnerCharacter)
	{
		//身体是否跟随摄像机旋转
		OwnerCharacter->bUseControllerRotationYaw = false;
		if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
		{
			//是否让让角色朝着移动的方向转头
			Move->bOrientRotationToMovement = true;
		}
	}

	if (OwnerPC)
	{
		//是否切断玩家鼠标对摄像机视角的控制
		OwnerPC->SetIgnoreLookInput(false);
	}

	ApplyLockOnCursorMode(false);
	SetComponentTickEnabled(false);
}

//判断当前锁定是否合法 DeltaSeconds遮挡计时
bool ULockOn::ValidateLockedTarget(float DeltaSeconds)
{
	AActor* Target = LockTarget.Get();
	if (!OwnerCharacter || !Target) return false;

	if (!IsTargetAlive(Target)) return false;

	//我的位置
	const FVector OwnerLoc = OwnerCharacter->GetActorLocation();
	//目标位置
	const FVector LockPoint = GetTargetLockPoint(Target);
	//Dist计算距离
	const float Dist = FVector::Dist(OwnerLoc, LockPoint);
	if (Dist > MaxDistanceCm) return false;

	if (!IsInKeepAngle(Target)) return false;

	VisibilityCheckTimer += DeltaSeconds;
	if (VisibilityCheckTimer >= VisibilityCheckInterval)
	{
		bLastKnownVisibility = IsVisibleToTarget(Target);
		VisibilityCheckTimer = 0.0f;
	}
	const bool bVisible = bLastKnownVisibility;

	if (bVisible)
	{
		OccludedAccum = 0.0f;
	}
	else
	{
		OccludedAccum += DeltaSeconds;
		if (OccludedAccum >= OcclusionBreakSeconds)
		{
			return false;
		}
	}

	return true;
}

//每帧对准目标  DeltaSeconds上一帧到这一帧经过了多少秒
void ULockOn::UpdateLockOn(float DeltaSeconds)
{
	if (!OwnerPC || !OwnerCharacter) return;

	AActor* Target = LockTarget.Get();
	if (!Target) return;

	FVector ViewLoc;
	//FRotator Pitch(俯仰角) Yaw(偏航角) Roll(翻滚角)
	FRotator ViewRot;
	//GetPlayerViewPoint获取摄像机的位置与角度
	OwnerPC->GetPlayerViewPoint(ViewLoc, ViewRot);

	const FVector LockPoint = GetTargetLockPoint(Target);
	//FindLookAtRotation:计算ViewLoc视角移动到LockPoint所需旋转的角度
	FRotator Desired = UKismetMathLibrary::FindLookAtRotation(ViewLoc, LockPoint);
	//Roll(翻滚角)水平
	Desired.Roll = 0.0f;
	//限制Pitch(俯仰角)的范围 Clamp限制至一个范围 
	Desired.Pitch = FMath::Clamp(Desired.Pitch, MinPitchDeg, MaxPitchDeg);
	//GetControlRotation获取摄像机的视角
	const FRotator Current = OwnerPC->GetControlRotation();
	//RInterpTo:自动计算最短路径
	const FRotator NewRot = FMath::RInterpTo(Current, Desired, DeltaSeconds, RotationInterpSpeed);
	//SetControlRotation设置摄像机的视角
	OwnerPC->SetControlRotation(NewRot);
}

//目标胸口锁点 Target目标
FVector ULockOn::GetTargetLockPoint(AActor* Target) const
{
	if (!Target) return FVector::ZeroVector;

	if (CachedTargetSocket != NAME_None)
	{
		USkeletalMeshComponent* Mesh = nullptr;
		if (const ACharacter* AsChar = Cast<ACharacter>(Target))
		{
			Mesh = AsChar->GetMesh();
		}
		else
		{
			Mesh = Target->FindComponentByClass<USkeletalMeshComponent>();
		}

		if (Mesh)
		{
			return Mesh->GetSocketLocation(CachedTargetSocket);
		}
	}

	return Target->GetActorLocation() + FVector(0, 0, FallbackChestZOffset);
}


bool ULockOn::IsInKeepAngle(AActor* Target) const
{
	// 角色或目标无效时，不能继续保持锁定。
	if (!IsValid(OwnerCharacter) || !IsValid(Target))
	{
		return false;
	}

	// 获取用于角度判断的朝向。
	// 正常情况下使用玩家镜头朝向；
	// 没有玩家控制器时，退回角色身体朝向。
	FRotator ViewRotation;

	if (IsValid(OwnerPC))
	{
		FVector ViewLocation;
		OwnerPC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewRotation = OwnerCharacter->GetActorRotation();
	}

	// 将镜头朝向投影到水平面。
	// 锁定保持角度只判断水平方向，不受目标高度影响。
	FVector Forward = ViewRotation.Vector();
	Forward.Z = 0.0f;

	// 当镜头接近垂直朝上或朝下时，
	// 投影到水平面后的向量长度可能接近零。
	if (!Forward.Normalize())
	{
		// 使用角色水平朝向作为兜底。
		Forward = OwnerCharacter->GetActorForwardVector();
		Forward.Z = 0.0f;

		if (!Forward.Normalize())
		{
			return false;
		}
	}

	// 获取从角色指向目标锁定点的方向。
	FVector ToTarget =
		GetTargetLockPoint(Target)
		- OwnerCharacter->GetActorLocation();

	// 同样只判断水平角度。
	ToTarget.Z = 0.0f;

	// 角色与目标在水平面位置重合时，
	// 不应仅因为方向向量无法归一化而解除锁定。
	if (!ToTarget.Normalize())
	{
		return true;
	}

	// 单位向量点积等于夹角的余弦值。
	const float Dot =
		FVector::DotProduct(Forward, ToTarget);

	// 防止运行时或蓝图中写入非法角度。
	const float ClampedHalfAngleDeg =
		FMath::Clamp(KeepHalfAngleDeg, 0.0f, 180.0f);

	// 将角度阈值转换为点积阈值。
	const float MinAllowedDot =
		FMath::Cos(
			FMath::DegreesToRadians(ClampedHalfAngleDeg)
		);

	return Dot >= MinAllowedDot;
}

bool ULockOn::IsVisibleToTarget(AActor* Target) const
{
	if (!OwnerPC || !OwnerCharacter || !Target) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	FVector ViewLoc;
	FRotator ViewRot;
	//GetPlayerViewPoint:获取摄像机的位置与朝向
	OwnerPC->GetPlayerViewPoint(ViewLoc, ViewRot);

	//计算目标各部位的世界位置
	const FVector Chest = GetTargetLockPoint(Target);
	const FVector Head = Chest + FVector(0, 0, 35.0f);
	const FVector Pelvis = Chest - FVector(0, 0, 35.0f);

	const FVector Points[3] = { Chest, Head, Pelvis };
	//声明一个参数结构体变量Params SCENE_QUERY_STAT性能分析	false:简单碰撞
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOnTrace), false);
	//AddIgnoredActor:添加忽略对象
	Params.AddIgnoredActor(OwnerCharacter);

	for (int32 i = 0; i < 3; ++i)
	{
		FHitResult Hit;
		//LineTraceSingleByChannel:直线监测,Hit击中目标,ViewLoc起点,Points[i]终点,VisibilityChannel检查通道,Params规则
		const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLoc, Points[i], VisibilityChannel, Params);
		//目标已经被锁定,所以如果没有打中,说明我与目标之间没有障碍物
		if (!bHit) return true;
		if (Hit.GetActor() == Target) return true;
	}

	return false;
}

//返回值:判断数据是否成功获取 1.屏幕中心位置 2.屏幕对角线 3.屏幕高度 4.屏幕长度
bool ULockOn::GetViewportInfo(FVector2D& OutCenter, float& OutDiag, int32& OutW, int32& OutH) const
{
	if (!OwnerPC) return false;

	int32 W = 0, H = 0;
	OwnerPC->GetViewportSize(W, H);
	if (W <= 0 || H <= 0) return false;

	OutW = W;
	OutH = H;
	//获取屏幕中心位置
	OutCenter = FVector2D(W * 0.5f, H * 0.5f);
	//去除分辨率不同导致的实际距离不同的错误,计算屏幕对角线长度,然后最终得分 =1-(偏离像素距离/计算屏幕对角线长度)
	OutDiag = FMath::Sqrt((float)W * (float)W + (float)H * (float)H);
	return true;
}

// 得分越高越优先锁定
float ULockOn::ScoreForAcquire(AActor* Candidate, const FVector2D& ScreenCenter, float ScreenDiag) const
{
	if (!OwnerPC || !OwnerCharacter || !Candidate) return -1e9f;

	FVector2D ScreenPos;
	if (!OwnerPC->ProjectWorldLocationToScreen(GetTargetLockPoint(Candidate), ScreenPos, true))
	{
		return -1e9f;
	}

	int32 W = 0, H = 0;
	OwnerPC->GetViewportSize(W, H);

	// 候选必须在屏幕内（准星优先的核心）
	if (ScreenPos.X < 0 || ScreenPos.X > W || ScreenPos.Y < 0 || ScreenPos.Y > H)
	{
		return -1e9f;
	}

	//计算得分逻辑
	const float ScreenDistNorm = (ScreenPos - ScreenCenter).Size() / FMath::Max(1.0f, ScreenDiag);
	const float CrosshairScore = 1.0f - FMath::Clamp(ScreenDistNorm, 0.0f, 1.0f);

	const float Dist = FVector::Dist(OwnerCharacter->GetActorLocation(), GetTargetLockPoint(Candidate));
	const float DistScore = 1.0f - FMath::Clamp(Dist / FMath::Max(1.0f, MaxDistanceCm), 0.0f, 1.0f);

	return CrosshairScore * WeightCrosshair + DistScore * WeightDistance;
}

FVector ULockOn::GetLockPointWorld() const
{
	return GetTargetLockPoint(LockTarget.Get());
}

bool ULockOn::GetLockPointScreen(FVector2D& OutScreenPos) const
{
	if (!OwnerPC) return false;
	return OwnerPC->ProjectWorldLocationToScreen(GetLockPointWorld(), OutScreenPos, true);
}

//钩子

bool ULockOn::CanSwitchLockTarget_Implementation() const
{
	return true;
}

bool ULockOn::IsTargetAlive_Implementation(AActor* Target) const
{
	return IsValid(Target);
}

bool ULockOn::IsValidLockCandidate_Implementation(AActor* Candidate) const
{
	//默认“宽松”：只要是Pawn且不是自己。你可在蓝图里加阵营/Tag/接口过滤。
	if (!Candidate || Candidate == OwnerCharacter) return false;
	return Cast<APawn>(Candidate) != nullptr;
}

