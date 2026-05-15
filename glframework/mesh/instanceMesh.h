#pragma once
#include"mesh.h"
#include<vector>
class InstanceMesh :public Mesh{
public:
	InstanceMesh(Geometry* geometry, Material* material, unsigned int InstanceCount);
	~InstanceMesh();
	void updateMatrices();
	void sortMatrices(glm::mat4 viewMatrix);
public:
	unsigned int mInstanceCount = 0;
	std::vector<glm::mat4> mInstanceMats = {};
	unsigned int mMatrixVbo{ 0 };

};