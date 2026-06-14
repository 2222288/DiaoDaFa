#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Attackif/AttackValid.h" 
#include "Math/UnrealMathUtility.h"
#include "Math/Vector2D.h"


class FCombatSampling
{
public:
	/** 缓存鼠标输入，并交给 AttackValid 判断是否形成一次有效攻击。 */
	void CacheMouseInput(const FVector2D& Input, float CurrentTime);

	/** 按下攻击键时开始采样攻击轨迹。 */
	void BeginAttackSampling(float CurrentTime);

	/** 松开攻击键时结束采样并清理输入缓存。 */
	void EndAttackSampling();

	/** 鼠标移动采样的最小阈值，低于该距离的输入会被过滤。 */
	float MinSampleDistance = 8.0f;

	/** 清理当前输入轨迹缓存。 */
	void ClearSamplingBuffer();

	/** 清理待定攻击数据。 */
	void ClearPendingAttack();

	/** 待定攻击轨迹评分。 */
	float PendingTrackScore = 0.0f;



};