#pragma once

//D3D12
#include <DirectXMath.h>

inline bool XM_CALLCONV IsNearZero(DirectX::FXMVECTOR vector)
{
	float sqLength= DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));

	if (sqLength < 1e-6f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool XM_CALLCONV IsNearParallel(DirectX::FXMVECTOR vectorA, DirectX::FXMVECTOR vectorB)
{
	DirectX::XMVECTOR normalizedA = DirectX::XMVector3Normalize(vectorA);
	DirectX::XMVECTOR normalizedB = DirectX::XMVector3Normalize(vectorB);

	float cosAngle = DirectX::XMVectorGetX(DirectX::XMVector3Cross(normalizedA, normalizedB));

	if (cosAngle < (- 1 + 1e-6f))
	{
		return true;
	}
	else if ((1 - 1e-6f) < cosAngle)
	{
		return true;
	}
	else
	{
		return false;
	}
}