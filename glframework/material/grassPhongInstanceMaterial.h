#pragma once
#include "material.h"
#include "../texture.h"

class GrassPhongInstanceMaterial :public Material {
public:
	GrassPhongInstanceMaterial();
	~GrassPhongInstanceMaterial();

public:
	Texture* mDiffuse{ nullptr };
	Texture* mSpecularMask{ nullptr };
	Texture* mOpacityMask{ nullptr };
	
	float		mShiness{ 1.0f };

	//草地贴图
	float     mUVScale{ 11.0f };
	float     mBrightNess{ 1.0f };

	//风力相关
	float  mWindScale{ 0.2f };
	glm::vec3 mWindDir = glm::vec3(1.0, 1.0, 1.0);

	//相位偏移
	float mPhiScale{ 1.0f };

	//云层阴影
	Texture* mCloudMask{ nullptr };
	glm::vec3 mCloudWhiteColor{ 0.0,0.0,0.0 };
	glm::vec3 mCloudBlackColor{ 0.0,1.0,0.0 };
	float mCloudUvScale{ 9.0f };
	float mCloudSpeed{ 0.1f };
	float mLerp_para{ 0.77f };
};