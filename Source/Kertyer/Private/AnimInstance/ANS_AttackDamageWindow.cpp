#include "AnimInstance/ANS_AttackDamageWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/AttackComponent.h"
#include "GameFramework/Actor.h"

void UANS_AttackDamageWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UAttackComponent* AttackComp = Owner->FindComponentByClass<UAttackComponent>();
	if (!AttackComp)
	{
		return;
	}

	AttackComp->EnableWeaponDamage();

	UE_LOG(LogTemp, Warning, TEXT("攻击有效帧开始：开启武器伤害碰撞"));
}

void UANS_AttackDamageWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UAttackComponent* AttackComp = Owner->FindComponentByClass<UAttackComponent>();
	if (!AttackComp)
	{
		return;
	}

	AttackComp->DisableWeaponDamage();

	UE_LOG(LogTemp, Warning, TEXT("攻击有效帧结束：关闭武器伤害碰撞"));
}