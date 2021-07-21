#include "Morpher.h"
#include <cmath>
#undef min
#undef max
#include "half.hpp"
#include "f4se/BSGeometry.h"

float round_v(float num)
{
	return (num > 0.0) ? floor(num + 0.5) : ceil(num - 0.5);
}

MorphApplicator::MorphApplicator(BSTriShape * _geometry, UInt8 * srcBlock, UInt8 * dstBlock, std::function<void(std::vector<Morpher::Vector3> &)> morph) : geometry(_geometry), morphFunc(morph)
{
	UInt64 vertexDesc = geometry->vertexDesc;
	UInt32 vertexSize = geometry->GetVertexSize();
	UInt32 blockSize = geometry->numVertices * vertexSize;
	BSGeometryData * geomData = geometry->geometryData;
	UInt32 numVertices = geometry->numVertices;

	// Pull the base data from the vertex block
	rawVertices.resize(numVertices);
	if(vertexDesc & BSTriShape::kFlag_UVs)
		rawUV.resize(numVertices);

	if(vertexDesc & BSTriShape::kFlag_Normals) {
		rawNormals.resize(numVertices);
		if(vertexDesc & BSTriShape::kFlag_Tangents) {
			rawTangents.resize(numVertices);
			rawBitangents.resize(numVertices);
		}
	}

	UInt8 * vertexBlock = srcBlock ? srcBlock : geomData->vertexData->vertexBlock;
	for(UInt32 i = 0; i < numVertices; i++)
	{
		UInt8 * vBegin = &vertexBlock[i * vertexSize];

		if(vertexDesc & BSTriShape::kFlag_FullPrecision)
		{
			rawVertices[i].x = (*(float *)vBegin); vBegin += 4;
			rawVertices[i].y = (*(float *)vBegin); vBegin += 4;
			rawVertices[i].z = (*(float *)vBegin); vBegin += 4;

			vBegin += 4; // Skip BitangetX
		}
		else
		{
			rawVertices[i].x = (*(half_float::half *)vBegin); vBegin += 2;
			rawVertices[i].y = (*(half_float::half *)vBegin); vBegin += 2;
			rawVertices[i].z = (*(half_float::half *)vBegin); vBegin += 2;

			vBegin += 2; // Skip BitangetX
		}

		if(vertexDesc & BSTriShape::kFlag_UVs)
		{
			rawUV[i].u = (*(half_float::half *)vBegin); vBegin += 2;
			rawUV[i].v = (*(half_float::half *)vBegin); vBegin += 2;
		}
	}

	morphFunc(rawVertices);

	Morpher::Triangle* triangles = nullptr;
	auto triangleData = geomData->triangleData;
	if(triangleData)
		triangles = (Morpher::Triangle*)triangleData->triangles;

	RecalcNormals(geometry->numTriangles, triangles);
	CalcTangentSpace(geometry->numTriangles, triangles);
	
	vertexBlock = dstBlock ? dstBlock : geomData->vertexData->vertexBlock;
	for(UInt32 i = 0; i < numVertices; i++)
	{
		UInt8 * vBegin = &vertexBlock[i * vertexSize];

		if(vertexDesc & BSTriShape::kFlag_FullPrecision)
		{
			(*(float *)vBegin) = rawVertices[i].x; vBegin += 4;
			(*(float *)vBegin) = rawVertices[i].y; vBegin += 4;
			(*(float *)vBegin) = rawVertices[i].z; vBegin += 4;

			(*(float *)vBegin) = rawBitangents[i].x; vBegin += 4;
		}
		else
		{
			(*(half_float::half *)vBegin) = rawVertices[i].x; vBegin += 2;
			(*(half_float::half *)vBegin) = rawVertices[i].y; vBegin += 2;
			(*(half_float::half *)vBegin) = rawVertices[i].z; vBegin += 2;

			(*(half_float::half *)vBegin) = rawBitangents[i].x; vBegin += 2;
		}

		// Skip UV write
		if(vertexDesc & BSTriShape::kFlag_UVs)
		{
			vBegin += 4;
		}

		if(vertexDesc & BSTriShape::kFlag_Normals)
		{
			*(SInt8*)vBegin = (UInt8)round_v((((rawNormals[i].x + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;
			*(SInt8*)vBegin = (UInt8)round_v((((rawNormals[i].y + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;
			*(SInt8*)vBegin = (UInt8)round_v((((rawNormals[i].z + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;

			*(SInt8*)vBegin = (UInt8)round_v((((rawBitangents[i].y + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;

			if(vertexDesc & BSTriShape::kFlag_Tangents)
			{
				*(SInt8*)vBegin = (UInt8)round_v((((rawTangents[i].x + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;
				*(SInt8*)vBegin = (UInt8)round_v((((rawTangents[i].y + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;
				*(SInt8*)vBegin = (UInt8)round_v((((rawTangents[i].z + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;

				*(SInt8*)vBegin = (UInt8)round_v((((rawBitangents[i].z + 1.0f) / 2.0f) * 255.0f)); vBegin += 1;
			}
		}
	}
}

void MorphApplicator::RecalcNormals(UInt32 numTriangles, Morpher::Triangle* triangles, const bool smooth, const float smoothThresh)
{
	UInt32 numVertices = rawVertices.size();

	std::vector<Morpher::Vector3> verts(numVertices);
	std::vector<Morpher::Vector3> norms(numVertices);

	for (UInt32 i = 0; i < numVertices; i++)
	{
		verts[i].x = rawVertices[i].x * -0.1f;
		verts[i].z = rawVertices[i].y * 0.1f;
		verts[i].y = rawVertices[i].z * 0.1f;
	}

	// Face normals
	Morpher::Vector3 tn;
	for (UInt32 t = 0; t < numTriangles; t++)
	{
		triangles[t].trinormal(verts, &tn);
		norms[triangles[t].p1] += tn;
		norms[triangles[t].p2] += tn;
		norms[triangles[t].p3] += tn;
	}

	for (auto &n : norms)
		n.Normalize();

	// Smooth normals
	if (smooth) {
		kd_matcher matcher(verts.data(), numVertices);
		for (int i = 0; i < matcher.matches.size(); i++)
		{
			std::pair<Morpher::Vector3*, int>& a = matcher.matches[i].first;
			std::pair<Morpher::Vector3*, int>& b = matcher.matches[i].second;

			Morpher::Vector3& an = norms[a.second];
			Morpher::Vector3& bn = norms[b.second];
			if (an.angle(bn) < smoothThresh * DEG2RAD) {
				Morpher::Vector3 anT = an;
				an += bn;
				bn += anT;
			}
		}

		for (auto &n : norms)
			n.Normalize();
	}

	for (UInt32 i = 0; i < numVertices; i++)
	{
		rawNormals[i].x = -norms[i].x;
		rawNormals[i].y = norms[i].z;
		rawNormals[i].z = norms[i].y;
	}
}

void MorphApplicator::CalcTangentSpace(UInt32 numTriangles, Morpher::Triangle * triangles)
{
	UInt32 numVertices = rawVertices.size();

	std::vector<Morpher::Vector3> tan1;
	std::vector<Morpher::Vector3> tan2;
	tan1.resize(numVertices);
	tan2.resize(numVertices);

	for (UInt32 i = 0; i < numTriangles; i++)
	{
		int i1 = triangles[i].p1;
		int i2 = triangles[i].p2;
		int i3 = triangles[i].p3;

		Morpher::Vector3 v1 = rawVertices[i1];
		Morpher::Vector3 v2 = rawVertices[i2];
		Morpher::Vector3 v3 = rawVertices[i3];

		Morpher::Vector2 w1 = rawUV[i1];
		Morpher::Vector2 w2 = rawUV[i2];
		Morpher::Vector2 w3 = rawUV[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.u - w1.u;
		float s2 = w3.u - w1.u;
		float t1 = w2.v - w1.v;
		float t2 = w3.v - w1.v;

		float r = (s1 * t2 - s2 * t1);
		r = (r >= 0.0f ? +1.0f : -1.0f);

		Morpher::Vector3 sdir = Morpher::Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		Morpher::Vector3 tdir = Morpher::Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		sdir.Normalize();
		tdir.Normalize();

		tan1[i1] += tdir;
		tan1[i2] += tdir;
		tan1[i3] += tdir;

		tan2[i1] += sdir;
		tan2[i2] += sdir;
		tan2[i3] += sdir;
	}

	for (UInt32 i = 0; i < numVertices; i++)
	{
		rawTangents[i] = tan1[i];
		rawBitangents[i] = tan2[i];

		if (rawTangents[i].IsZero() || rawBitangents[i].IsZero())
		{
			rawTangents[i].x = rawNormals[i].y;
			rawTangents[i].y = rawNormals[i].z;
			rawTangents[i].z = rawNormals[i].x;
			rawBitangents[i] = rawNormals[i].cross(rawTangents[i]);
		}
		else
		{
			rawTangents[i].Normalize();
			rawTangents[i] = (rawTangents[i] - rawNormals[i] * rawNormals[i].dot(rawTangents[i]));
			rawTangents[i].Normalize();

			rawBitangents[i].Normalize();

			rawBitangents[i] = (rawBitangents[i] - rawNormals[i] * rawNormals[i].dot(rawBitangents[i]));
			rawBitangents[i] = (rawBitangents[i] - rawTangents[i] * rawTangents[i].dot(rawBitangents[i]));

			rawBitangents[i].Normalize();
		}
	}
}