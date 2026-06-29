// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseActor/Harm.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"


AHarm::AHarm()
{

	PrimaryActorTick.bCanEverTick = false;

    // 1. 创建网格体作为根组件
    TrapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrapMesh"));
    RootComponent = TrapMesh;

    // 2. 创建碰撞盒，并挂在网格体上
    DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
    DamageBox->SetupAttachment(GetRootComponent());

    // 设置碰撞盒大小 (根据您的模型大小调整)
    DamageBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

    // 3. 设置碰撞预设：只检测重叠 (Overlap)，不阻挡物理
    DamageBox->SetCollisionProfileName(TEXT("Trigger"));

}


void AHarm::BeginPlay()
{
	Super::BeginPlay();
	
    // 绑定重叠事件：当有人进入盒子时，执行 OnOverlapBegin
    DamageBox->OnComponentBeginOverlap.AddDynamic(this, &AHarm::OnOverlapBegin);

}
    

void AHarm::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHarm::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

    // 排除掉陷阱自己，防止自己伤害自己
    if (OtherActor && OtherActor != this)
    {
        // 核心逻辑：对进入范围的 Actor 施加伤害
        // 这会自动触发我们在 Base.cpp 里写的 TakeDamage 函数
        UGameplayStatics::ApplyDamage(
            OtherActor,           // 受害者
            DamageAmount,         // 伤害值
            GetInstigatorController(), // 施害者控制器 (如果是环境陷阱可以是nullptr)
            this,                 // 伤害来源 (陷阱本身)
            UDamageType::StaticClass() // 伤害类型 (默认)
        );

        // 打印调试信息
        UE_LOG(LogTemp, Warning, TEXT("陷阱对 %s 造成了 %f 点伤害"), *OtherActor->GetName(), DamageAmount);
    }
}

