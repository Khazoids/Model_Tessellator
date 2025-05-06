#pragma once
#include "cyVector.h"

class ModelState
{
public:
	cyVec3f rotation;
	cyVec3f translation;
	cyVec3f scale;

	ModelState() : rotation(cyVec3f(0.0, 0.0, 0.0)), translation(cyVec3f(0.0, 0.0, 0.0)), scale(cyVec3f(1.0, 1.0, 1.0)) {}
	ModelState(cyVec3f rotation, cyVec3f translation, cyVec3f scale) : rotation(rotation), translation(translation), scale(scale) {}

	~ModelState() {}


	void IncrementRotation(float x, float y, float z) { rotation.Set(rotation.x + x, rotation.y + y, rotation.z + z); }
	void IncrementTranslation(float x, float y, float z) { translation.Set(translation.x + x, translation.y + y, translation.z + z); }
	void PrintState()
	{	
		std::cout << "Rotation: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << rotation[i] << " ";
		}

		std::cout << "\t";

		std::cout << "Translation: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << translation[i] << " ";
		}

		std::cout << "\t";

		std::cout << "Scale: ";
		for (int i = 0; i < 3; i++)
		{
			std::cout << scale[i] << " ";
		}

		std::cout << std::endl;
	}
};

