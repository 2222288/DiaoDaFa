#include "Widget/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHealthBarWidget::SetHealthPercent(float Percent)
{
	if (!HealthProgressBar)
	{
		return;
	}

	HealthProgressBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
}