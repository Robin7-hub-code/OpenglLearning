#include "instanceMesh.h"
#include<algorithm>

InstanceMesh::InstanceMesh(Geometry* geometry, Material* material, unsigned int InstanceCount):Mesh(geometry,material)
{
	mType = ObjectType::InstanceMesh;
	mInstanceCount =InstanceCount ;
	mInstanceMats.resize(InstanceCount);

	glGenBuffers(1, &mMatrixVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mMatrixVbo);
	glBufferData(GL_ARRAY_BUFFER, mInstanceCount * sizeof(glm::mat4), mInstanceMats.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(geometry->getVao());
	glBindBuffer(GL_ARRAY_BUFFER, mMatrixVbo);
	for (int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(4+i);
		glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(float)*4*i));
		glVertexAttribDivisor(4 + i, 1);
	}
	glBindVertexArray(0);
}

InstanceMesh::~InstanceMesh()
{

}

void InstanceMesh::updateMatrices()
{
	glBindBuffer(GL_ARRAY_BUFFER, mMatrixVbo);
	glBufferSubData(GL_ARRAY_BUFFER,0, sizeof(glm::mat4) * mInstanceCount, mInstanceMats.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void InstanceMesh::sortMatrices(glm::mat4 viewMatrix)
{
	std::sort(

		mInstanceMats.begin(),
		mInstanceMats.end(),
		[viewMatrix](const glm::mat4& a, const glm::mat4& b) {

			//1 计算a的相机系的Z0
			auto modelMatrixA = a;
			auto worldPositionA = modelMatrixA * glm::vec4(0.0, 0.0, 0.0, 1.0);
			auto cameraPositionA = viewMatrix * worldPositionA;

			//2 计算b的相机系的Z
			auto modelMatrixB = b;
			auto worldPositionB = modelMatrixB * glm::vec4(0.0, 0.0, 0.0, 1.0);
			auto cameraPositionB = viewMatrix * worldPositionB;

			return cameraPositionA.z < cameraPositionB.z;
		}
	);

}
