#pragma once

//Podo
#include "Material.h"
#include "Mesh.h"

class Asset
{
public:

	Asset() = default;
	Asset(Asset&& sourceAsset) noexcept = default;
	Asset& operator=(Asset&& sourceAsset) noexcept = default;
	~Asset() = default;

	//NOTE: 의도치 않은 얕은 복사나 참조 증가가 일어나지 않도록 복사를 금지함
	Asset(const Asset& sourceAsset) = delete;
	Asset& operator=(const Asset& sourceAsset) = delete;

	//NOTE: 메쉬는 업데이트가 이뤄지지 않으므로 const로 받아 저장함
	void SetMeshPtr(const Mesh* pMesh)
	{
		m_pMesh = pMesh;
	}

	void SetMaterialPtr(Material* pMaterial)
	{
		m_pMaterial = pMaterial;
	}

	const Mesh* GetMesh() const
	{
		return m_pMesh;
	}

	Material* GetMaterial() const
	{
		return m_pMaterial;
	}

private:

	const Mesh* m_pMesh		= {};
	Material* m_pMaterial	= {};
};