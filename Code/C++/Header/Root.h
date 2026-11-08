//NOTE: 오브젝트가 텍스처나 머테리얼보다 더 자주 인자 변경이 일어나지만, 가독성을 위해 대상들의 논리적인 순서를 따르기로 함
enum RootParameter
{
	TEXTURE_CONSTANT,
	MATERIAL_CONSTANT,
	OBJECT_CONSTANT,
	LIGHT_CONSTANT,
	CAMERA_CONSTANT,

	ROOT_PARAMETER_COUNT
};