// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Treat.generated.h"

UCLASS()
class KERTYER_API ATreat : public AActor
{
	GENERATED_BODY()
	
public:	

	ATreat();

    virtual void Tick(float DeltaTime) override;
protected:

    // 静态网格体 (用来显示陷阱的样子，比如地刺)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap Properties")
    TObjectPtr<class UStaticMeshComponent> MeshComp;

    // 碰撞盒 (用来检测有没有人踩上去)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trap Properties")
    TObjectPtr<class UBoxComponent> TreatBox;

    // 伤害数值 (可以在编辑器里配置)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap Properties")
    float Treatmentamount = 20.0f;

	virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnHealOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

 };
