// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseActor/Treat.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/Base.h"


ATreat::ATreat()
{
 	
    PrimaryActorTick.bCanEverTick = false;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;

    TreatBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TreatBox->SetupAttachment(RootComponent);

    // 设置碰撞预设为 Trigger，专用于检测
    TreatBox->SetCollisionProfileName(TEXT("Trigger"));
    TreatBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

}


void ATreat::BeginPlay()
{
	Super::BeginPlay();
	
    // 绑定重叠事件：当有人进入盒子时，执行 OnOverlapBegin
    if (TreatBox)
    {
        TreatBox->OnComponentBeginOverlap.AddDynamic(this, &ATreat::OnHealOverlap);
    }
}


void ATreat::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATreat::OnHealOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 1. 检查是否有效
    if (!OtherActor || OtherActor == this) return;

    // 2. 尝试把进入的物体转换成 ABase (只有 Base 及其子类才有血条)
    ABase* BaseCharacter = Cast<ABase>(OtherActor);

    if (BaseCharacter)
    {
        // 3. 调用 Base 里写好的 Heal 函数
        BaseCharacter->Treat(Treatmentamount);

        UE_LOG(LogTemp, Warning, TEXT(">> [C++治疗] 对 %s 进行了治疗，数值: %f"), *OtherActor->GetName(), Treatmentamount);

    }
}