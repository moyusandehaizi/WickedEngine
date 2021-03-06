#pragma once
#include "CommonInclude.h"
#include "wiSceneComponents.h"

namespace wiSceneComponents
{
	struct Camera;
}

class Translator : public wiSceneComponents::Transform
{
private:
	XMFLOAT4 prevPointer;
	XMFLOAT4X4 dragStart, dragEnd;
	bool dragging;
	bool dragStarted;
	bool dragEnded;
public:
	Translator();
	~Translator();

	void Update();
	void Draw(wiSceneComponents::Camera* camera, GRAPHICSTHREAD threadID);

	bool enabled;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state;

	float dist;

	bool isTranslator, isScalator, isRotator;


	// Check if the drag started in this exact frame
	bool IsDragStarted();
	XMFLOAT4X4 GetDragStart();
	// Check if the drag ended in this exact frame
	bool IsDragEnded();
	XMFLOAT4X4 GetDragEnd();
};

