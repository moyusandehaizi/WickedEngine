#include "stdafx.h"
#include "Translator.h"
#include "wiRenderer.h"
#include "wiInputManager.h"
#include "wiMath.h"

using namespace wiGraphicsTypes;
using namespace wiSceneComponents;

GraphicsPSO* pso_solidpart = nullptr;
GraphicsPSO* pso_wirepart = nullptr;
wiGraphicsTypes::GPUBuffer* vertexBuffer_Axis = nullptr;
wiGraphicsTypes::GPUBuffer* vertexBuffer_Plane = nullptr;
wiGraphicsTypes::GPUBuffer* vertexBuffer_Origin = nullptr;
int vertexCount_Axis = 0;
int vertexCount_Plane = 0;
int vertexCount_Origin = 0;


Translator::Translator() :Transform()
{
	prevPointer = XMFLOAT4(0, 0, 0, 0);

	XMStoreFloat4x4(&dragStart, XMMatrixIdentity());
	XMStoreFloat4x4(&dragEnd, XMMatrixIdentity());

	dragging = false;
	dragStarted = false;
	dragEnded = false;

	enabled = true;

	state = TRANSLATOR_IDLE;

	dist = 1;

	isTranslator = true; isScalator = false; isRotator = false;


	GraphicsDevice* device = wiRenderer::GetDevice();

	if (pso_solidpart == nullptr)
	{
		GraphicsPSODesc desc;

		desc.vs = wiRenderer::vertexShaders[VSTYPE_LINE];
		desc.ps = wiRenderer::pixelShaders[PSTYPE_LINE];
		desc.il = wiRenderer::vertexLayouts[VLTYPE_LINE];
		desc.dss = wiRenderer::depthStencils[DSSTYPE_XRAY];
		desc.rs = wiRenderer::rasterizers[RSTYPE_DOUBLESIDED];
		desc.bs = wiRenderer::blendStates[BSTYPE_ADDITIVE];
		desc.pt = TRIANGLELIST;

		pso_solidpart = new GraphicsPSO;
		device->CreateGraphicsPSO(&desc, pso_solidpart);
	}

	if (pso_wirepart == nullptr)
	{
		GraphicsPSODesc desc;

		desc.vs = wiRenderer::vertexShaders[VSTYPE_LINE];
		desc.ps = wiRenderer::pixelShaders[PSTYPE_LINE];
		desc.il = wiRenderer::vertexLayouts[VLTYPE_LINE];
		desc.dss = wiRenderer::depthStencils[DSSTYPE_XRAY];
		desc.rs = wiRenderer::rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
		desc.bs = wiRenderer::blendStates[BSTYPE_TRANSPARENT];
		desc.pt = LINELIST;

		pso_wirepart = new GraphicsPSO;
		device->CreateGraphicsPSO(&desc, pso_wirepart);
	}

	if (vertexBuffer_Axis == nullptr)
	{
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(3,0,0,1), XMFLOAT4(1,1,1,1),
			};
			vertexCount_Axis = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Axis = new GPUBuffer;
			device->CreateBuffer(&bd, &InitData, vertexBuffer_Axis);
		}
	}

	if (vertexBuffer_Plane == nullptr)
	{
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,1,0,1), XMFLOAT4(1,1,1,1),

				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,1,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(0,1,0,1), XMFLOAT4(1,1,1,1),
			};
			vertexCount_Plane = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Plane = new GPUBuffer;
			device->CreateBuffer(&bd, &InitData, vertexBuffer_Plane);
		}
	}

	if (vertexBuffer_Origin == nullptr)
	{
		{
			float edge = 0.2f;
			XMFLOAT4 verts[] = {
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
			};
			vertexCount_Origin = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Origin = new GPUBuffer;
			device->CreateBuffer(&bd, &InitData, vertexBuffer_Origin);
		}
	}
}

Translator::~Translator()
{
}

void Translator::Update()
{
	dragStarted = false;
	dragEnded = false;

	XMFLOAT4 pointer = wiInputManager::GetInstance()->getpointer();
	Camera* cam = wiRenderer::getCamera();
	XMVECTOR pos = XMLoadFloat3(&translation);

	if (enabled)
	{

		if (!dragging)
		{
			XMMATRIX P = cam->GetProjection();
			XMMATRIX V = cam->GetView();
			XMMATRIX W = XMMatrixIdentity();

			dist = wiMath::Distance(translation, cam->translation) * 0.05f;

			XMVECTOR o, x, y, z, p, xy, xz, yz;

			o = pos;
			x = o + XMVectorSet(3, 0, 0, 0) * dist;
			y = o + XMVectorSet(0, 3, 0, 0) * dist;
			z = o + XMVectorSet(0, 0, 3, 0) * dist;
			p = XMLoadFloat4(&pointer);
			xy = o + XMVectorSet(0.5f, 0.5f, 0, 0) * dist;
			xz = o + XMVectorSet(0.5f, 0, 0.5f, 0) * dist;
			yz = o + XMVectorSet(0, 0.5f, 0.5f, 0) * dist;


			o = XMVector3Project(o, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			x = XMVector3Project(x, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			y = XMVector3Project(y, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			z = XMVector3Project(z, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			xy = XMVector3Project(xy, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			xz = XMVector3Project(xz, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			yz = XMVector3Project(yz, 0, 0, cam->width, cam->height, 0, 1, P, V, W);

			XMVECTOR oDisV = XMVector3Length(o - p);
			XMVECTOR xyDisV = XMVector3Length(xy - p);
			XMVECTOR xzDisV = XMVector3Length(xz - p);
			XMVECTOR yzDisV = XMVector3Length(yz - p);

			float xDis = wiMath::GetPointSegmentDistance(p, o, x);
			float yDis = wiMath::GetPointSegmentDistance(p, o, y);
			float zDis = wiMath::GetPointSegmentDistance(p, o, z);
			float oDis = XMVectorGetX(oDisV);
			float xyDis = XMVectorGetX(xyDisV);
			float xzDis = XMVectorGetX(xzDisV);
			float yzDis = XMVectorGetX(yzDisV);

			if (oDis < 10)
			{
				state = TRANSLATOR_XYZ;
			}
			else if (xyDis < 20)
			{
				state = TRANSLATOR_XY;
			}
			else if (xzDis < 20)
			{
				state = TRANSLATOR_XZ;
			}
			else if (yzDis < 20)
			{
				state = TRANSLATOR_YZ;
			}
			else if (xDis < 10)
			{
				state = TRANSLATOR_X;
			}
			else if (yDis < 10)
			{
				state = TRANSLATOR_Y;
			}
			else if (zDis < 10)
			{
				state = TRANSLATOR_Z;
			}
			else if (!dragging)
			{
				state = TRANSLATOR_IDLE;
			}
		}

		if (dragging || (state != TRANSLATOR_IDLE && wiInputManager::GetInstance()->down(VK_LBUTTON)))
		{
			XMVECTOR plane, planeNormal;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR axis = XMVectorSet(1, 0, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR axis = XMVectorSet(0, 1, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR axis = XMVectorSet(0, 0, 1, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_XY)
			{
				planeNormal = XMVectorSet(0, 0, 1, 0);
			}
			else if (state == TRANSLATOR_XZ)
			{
				planeNormal = XMVectorSet(0, 1, 0, 0);
			}
			else if (state == TRANSLATOR_YZ)
			{
				planeNormal = XMVectorSet(1, 0, 0, 0);
			}
			else
			{
				// xyz
				planeNormal = cam->GetAt();
			}
			plane = XMPlaneFromPointNormal(pos, XMVector3Normalize(planeNormal));

			RAY ray = wiRenderer::getPickRay((long)pointer.x, (long)pointer.y);
			XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
			XMVECTOR rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersection = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam->zFarP);

			ray = wiRenderer::getPickRay((long)prevPointer.x, (long)prevPointer.y);
			rayOrigin = XMLoadFloat3(&ray.origin);
			rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersectionPrev = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam->zFarP);

			XMVECTOR deltaV;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(1, 0, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 1, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 0, 1, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else
			{
				deltaV = intersection - intersectionPrev;

				if (isScalator)
				{
					deltaV = XMVectorSplatY(deltaV);
				}
			}

			XMFLOAT3 delta;
			if (isRotator)
			{
				deltaV /= XMVector3Length(intersection - rayOrigin);
				deltaV *= XM_2PI;
			}
			XMStoreFloat3(&delta, deltaV);


			XMMATRIX transf = XMMatrixIdentity();

			if (isTranslator)
			{
				transf *= XMMatrixTranslation(delta.x, delta.y, delta.z);
			}
			if (isRotator)
			{
				transf *= XMMatrixRotationRollPitchYaw(delta.x, delta.y, delta.z);
			}
			if (isScalator)
			{
				transf *= XMMatrixScaling((1.0f / scale.x) * (scale.x + delta.x), (1.0f / scale.y) * (scale.y + delta.y), (1.0f / scale.z) * (scale.z + delta.z));
			}

			Transform::transform(transf);

			Transform::applyTransform();

			if (!dragging)
			{
				dragStarted = true;
				dragStart = world;
			}

			dragging = true;
		}

		if (!wiInputManager::GetInstance()->down(VK_LBUTTON))
		{
			if (dragging)
			{
				dragEnded = true;
				dragEnd = world;
			}
			dragging = false;
			Transform::UpdateTransform();
		}

	}
	else
	{
		if (dragging)
		{
			dragEnded = true;
			dragEnd = world;
		}
		dragging = false;
		Transform::UpdateTransform();
	}

	prevPointer = pointer;
}
void Translator::Draw(Camera* camera, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Editor - Translator", threadID);

	XMMATRIX VP = camera->GetViewProjection();

	wiRenderer::MiscCB sb;

	XMMATRIX mat = XMMatrixScaling(dist, dist, dist)*XMMatrixTranslation(translation.x, translation.y, translation.z) * VP;
	XMMATRIX matX = XMMatrixTranspose(mat);
	XMMATRIX matY = XMMatrixTranspose(XMMatrixRotationZ(XM_PIDIV2)*XMMatrixRotationY(XM_PIDIV2)*mat);
	XMMATRIX matZ = XMMatrixTranspose(XMMatrixRotationY(-XM_PIDIV2)*XMMatrixRotationZ(-XM_PIDIV2)*mat);

	// Planes:
	{
		device->BindGraphicsPSO(pso_solidpart, threadID);
		GPUBuffer* vbs[] = {
			vertexBuffer_Plane,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
	}

	// xy
	sb.mTransform = matX;
	sb.mColor = state == TRANSLATOR_XY ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Plane, 0, threadID);

	// xz
	sb.mTransform = matZ;
	sb.mColor = state == TRANSLATOR_XZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Plane, 0, threadID);

	// yz
	sb.mTransform = matY;
	sb.mColor = state == TRANSLATOR_YZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.2f, 0.2f, 0, 0.2f);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Plane, 0, threadID);

	// Lines:
	{
		device->BindGraphicsPSO(pso_wirepart, threadID);
		GPUBuffer* vbs[] = {
			vertexBuffer_Axis,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
	}

	// x
	sb.mTransform = matX;
	sb.mColor = state == TRANSLATOR_X ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(1, 0, 0, 1);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Axis, 0, threadID);

	// y
	sb.mTransform = matY;
	sb.mColor = state == TRANSLATOR_Y ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 1, 0, 1);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Axis, 0, threadID);

	// z
	sb.mTransform = matZ;
	sb.mColor = state == TRANSLATOR_Z ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 0, 1, 1);
	device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
	device->Draw(vertexCount_Axis, 0, threadID);

	// Origin:
	{
		device->BindGraphicsPSO(pso_solidpart, threadID);
		GPUBuffer* vbs[] = {
			vertexBuffer_Origin,
		};
		const UINT strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		sb.mTransform = XMMatrixTranspose(mat);
		sb.mColor = state == TRANSLATOR_XYZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
		device->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &sb, threadID);
		device->Draw(vertexCount_Origin, 0, threadID);
	}

	device->EventEnd(threadID);
}

bool Translator::IsDragStarted()
{
	return dragStarted;
}
XMFLOAT4X4 Translator::GetDragStart()
{
	return dragStart;
}
bool Translator::IsDragEnded()
{
	return dragEnded;
}
XMFLOAT4X4 Translator::GetDragEnd()
{
	return dragEnd;
}
