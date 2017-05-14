#pragma once

#include "shape.hpp"
#include "kd_matcher.hpp"

#include <vector>
#include <functional>

class BSTriShape;

class MorphApplicator
{
public:
	MorphApplicator(BSTriShape * _geometry, UInt8 * srcBlock, UInt8 * dstBlock, std::function<void(std::vector<Morpher::Vector3> &)> morph);

	void RecalcNormals(UInt32 numTriangles, Morpher::Triangle* triangles, const bool smooth = true, const float smoothThres = 60.0f);
	void CalcTangentSpace(UInt32 numTriangles, Morpher::Triangle * triangles);

protected:
	BSTriShape * geometry;
	std::function<void(std::vector<Morpher::Vector3> &)> morphFunc;
	std::vector<Morpher::Vector3> rawVertices;
	std::vector<Morpher::Vector3> rawNormals;
	std::vector<Morpher::Vector2> rawUV;
	std::vector<Morpher::Vector3> rawTangents;
	std::vector<Morpher::Vector3> rawBitangents;
};
