#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class KERTYER_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//设置血条百分比
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetHealthPercent(float Percent);

protected:
	//血条进度条组件
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;
};